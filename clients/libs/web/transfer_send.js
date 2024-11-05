import axios from 'axios';
import initWasm from 'mercury-wasm';
import wasmUrl from 'mercury-wasm/mercury_wasm_bg.wasm?url'
import * as mercury_wasm from 'mercury-wasm';
import storageManager from './storage_manager.js';
import utils from './utils.js';
import CoinStatus from './coin_enum.js';
import deposit from './deposit.js';
import transaction from './transaction.js';

async function createBackupTransactions(
    clientConfig,
    recipientAddress,
    wallet,
    statechainId,
    duplicatedIndexes
) {

    // Validate duplicated indexes
    if (duplicatedIndexes) {
        for (const index of duplicatedIndexes) {
            if (index >= wallet.coins.length) {
                throw new Error(`Index ${index} does not exist in wallet.coins`);
            }
        }
    }

    let coinList = [];

    const backupTransactions = storageManager.getBackupTransactions(wallet.name, statechainId);

    // Get coins that already have a backup transaction
    for (const coin of wallet.coins) {
        // Check if coin matches any backup transaction and has one of the specified statuses
        const hasMatchingTx = backupTransactions.some(backupTx => {
            try {
                const txOutpoint = mercury_wasm.getPreviousOutpoint(backupTx);

                if (coin.utxo_txid && coin.utxo_vout !== undefined) {
                    return (
                        (coin.status === CoinStatus.DUPLICATED ||
                         coin.status === CoinStatus.CONFIRMED ||
                         coin.status === CoinStatus.IN_TRANSFER) &&
                        txOutpoint.txid === coin.utxo_txid &&
                        txOutpoint.vout === coin.utxo_vout
                    );
                }
                return false;
            } catch (error) {
                console.log(error);
                return false;
            }
        });

        let coinToAdd = false;

        if (duplicatedIndexes) {
            if (coin.statechain_id === statechainId && 
                (coin.status === CoinStatus.CONFIRMED || coin.status === CoinStatus.IN_TRANSFER)) {
                coinToAdd = true;
            }

            if (coin.statechain_id === statechainId && 
                coin.status === CoinStatus.DUPLICATED && 
                duplicatedIndexes.includes(coin.duplicate_index)) {
                coinToAdd = true;
            }
        }

        if (hasMatchingTx || coinToAdd) {
            if (!coin.locktime) {
                throw new Error("coin.locktime is undefined or null");
            }

            const response = await axios.get(`${clientConfig.esploraServer}/api/blocks/tip/height`);
            const block_header = response.data;
            const currentBlockheight = parseInt(block_header, 10);

            if (isNaN(currentBlockheight)) {
                throw new Error(`Invalid block height: ${block_header}`);
            }

            if (currentBlockheight > coin.locktime) {
                throw new Error(
                    `The coin is expired. Coin locktime is ${coin.locktime} and current blockheight is ${currentBlockheight}`
                );
            }

            coinList.push(coin);
        }
    }

    // Validate coins with zero index
    const coinsWithZeroIndex = coinList.filter(
        coin => coin.duplicate_index === 0 && 
        (coin.status === CoinStatus.CONFIRMED || coin.status === CoinStatus.IN_TRANSFER)
    );

    if (coinsWithZeroIndex.length !== 1) {
        throw new Error("There must be at least one coin with duplicate_index == 0");
    }

    for (let coin of coinList) {
        if (coin.status === 'DUPLICATED') {
            try {
                let response = await axios.get(`${clientConfig.esploraServer}/api/address/${coin.aggregated_address}/utxo`);
                let utxo_list = response.data;

                let utxo = null;

                for (let unspent of utxo_list) {
                    if (coin.utxo_txid === unspent.txid && coin.utxo_vout === unspent.vout) {
                        utxo = unspent;
                        break;
                    }
                }

                if (utxo) {

                    let isConfirmed =  false;

                    if (utxo.status.confirmed) {
                        const response = await axios.get(`${clientConfig.esploraServer}/api/blocks/tip/height`);
                        const block_header = response.data;
                        const blockheight = parseInt(block_header, 10);

                        if (isNaN(blockheight)) {
                            throw new Error(`Invalid block height: ${block_header}`);
                        }

                        const confirmations = blockheight - parseInt(utxo.status.block_height, 10) + 1;

                        const confirmationTarget = clientConfig.confirmationTarget;

                        isConfirmed = confirmations >= confirmationTarget;
                    }

                    if (!isConfirmed) {
                        throw new Error(`The coin with duplicated index ${coin.duplicate_index} has not yet been confirmed. This transfer cannot be performed.`);
                    }
                    
                }
            } catch (error) {
                throw error;
            }
        }
    }
                
    // Move the coin with CONFIRMED status to the first position
    coinList.sort((a, b) => {
        if (a.status === CoinStatus.CONFIRMED) return -1;
        if (b.status === CoinStatus.CONFIRMED) return 1;
        return 0;
    });

    const newBackupTransactions = [];

    // Create backup transaction for every coin
    const backupTxs = storageManager.getBackupTransactions(wallet.name, statechainId);
    let newTxN = backupTxs.length;

    for (const coin of coinList) {
        let filteredTransactions = [];

        for (const backupTx of backupTxs) {
            try {
                const txOutpoint = mercury_wasm.getPreviousOutpoint(backupTx);
                if (coin.utxo_txid && coin.utxo_vout !== undefined) {
                    if (txOutpoint.txid === coin.utxo_txid && txOutpoint.vout === coin.utxo_vout) {
                        filteredTransactions.push({ ...backupTx });
                    }
                }
            } catch {
                continue;
            }
        }

        filteredTransactions.sort((a, b) => a.tx_n - b.tx_n);

        if (filteredTransactions.length === 0) {
            newTxN++;
            const bkpTx1 = await deposit.createTx1(clientConfig, coin, wallet.network, newTxN);
            filteredTransactions.push(bkpTx1);
        }

        const qtBackupTx = filteredTransactions.length;
        newTxN++;

        const bkpTx1 = filteredTransactions[0];

        const block_height = mercury_wasm.getBlockheight(bkpTx1);

        const serverInfo = await utils.infoConfig(clientConfig);

        let feeRateSatsPerByte = (serverInfo.feeRateSatsPerByte > clientConfig.maxFeeRate) ? clientConfig.maxFeeRate: serverInfo.feeRateSatsPerByte;

        const isWithdrawal = false;

        const signedTx = await transaction.newTransaction(
            clientConfig, 
            coin, 
            recipientAddress, 
            isWithdrawal, 
            qtBackupTx, 
            block_height, 
            wallet.network,
            feeRateSatsPerByte,
            serverInfo.initlock,
            serverInfo.interval
        );

        const backupTx = {
            tx_n: newTxN,
            tx: signedTx,
            client_public_nonce: coin.public_nonce,
            server_public_nonce: coin.server_public_nonce,
            client_public_key: coin.user_pubkey,
            server_public_key: coin.server_pubkey,
            blinding_factor: coin.blinding_factor
        };

        filteredTransactions.push(backupTx);

        if (coin.duplicate_index === 0) {
            newBackupTransactions.unshift(...filteredTransactions);
        } else {
            newBackupTransactions.push(...filteredTransactions);
        }

        coin.status = CoinStatus.IN_TRANSFER;
    }

    newBackupTransactions.sort((a, b) => a.txN - b.txN);

    return newBackupTransactions;
}

const execute = async (clientConfig, walletName, statechainId, toAddress, forceSend, batchId, duplicatedIndexes)  => {

    await initWasm(wasmUrl);

    let wallet = storageManager.getWallet(walletName);

    const isCoinDuplicated = wallet.coins.some(c => 
        c.statechain_id === statechainId &&
        c.status === CoinStatus.DUPLICATED
    );

    if (isCoinDuplicated && !forceSend) {
        throw new Error("Coin is duplicated. If you want to proceed, use the command '--force, -f' option. " +
            "You will no longer be able to move other duplicate coins with the same statechain_id and this will cause PERMANENT LOSS of these duplicate coin funds.");
    }

    const areThereDuplicateCoinsWithdrawn = wallet.coins.some(c => 
        c.statechain_id === statechainId &&
        (c.status === CoinStatus.WITHDRAWING || c.status === CoinStatus.WITHDRAWN) &&
        c.duplicate_index > 0
    );

    if (areThereDuplicateCoinsWithdrawn) {
        throw new Error("There have been withdrawals of other coins with this same statechain_id (possibly duplicates). " +
            "This transfer cannot be performed because the recipient would reject it due to the difference in signature count. " +
            "This coin can be withdrawn, however."
        );
    }

    let coinsWithStatechainId = wallet.coins.filter(c => {
        return c.statechain_id === statechainId && 
            (c.status == CoinStatus.CONFIRMED || c.status == CoinStatus.IN_TRANSFER) && 
            c.duplicate_index === 0;
    });

    if (!coinsWithStatechainId || coinsWithStatechainId.length === 0) {
        throw new Error(`No coins with status CONFIRMED or IN_TRANSFER associated with this statechain ID ${statechainId} were found.`);
    }

    // If the user sends to himself, he will have two coins with same statechain_id
    // In this case, we need to find the one with the lowest locktime
    // Sort the coins by locktime in ascending order and pick the first one
    let coin = coinsWithStatechainId.sort((a, b) => a.locktime - b.locktime)[0];

    const statechain_id = coin.statechain_id;
    const signed_statechain_id = coin.signed_statechain_id;

    const decodedTransferAddress = mercury_wasm.decodeTransferAddress(toAddress);
    const new_auth_pubkey = decodedTransferAddress.auth_pubkey;

    const new_x1 = await get_new_x1(clientConfig, statechain_id, signed_statechain_id, new_auth_pubkey, batchId);

    const input_txid = coin.utxo_txid;
    const input_vout = coin.utxo_vout;
    const client_seckey = coin.user_privkey;
    const coin_amount = coin.amount;
    const recipient_address = toAddress;

    const transfer_signature = mercury_wasm.createTransferSignature(recipient_address, input_txid, input_vout, client_seckey);

    const backupTxs = await createBackupTransactions(clientConfig, recipient_address, wallet, statechain_id, duplicatedIndexes);

    const transferUpdateMsgRequestPayload = mercury_wasm.createTransferUpdateMsg(new_x1, recipient_address, coin, transfer_signature, backupTxs);

    const statechain_entity_url = clientConfig.statechainEntity;
    const path = "transfer/update_msg";
    const url = statechain_entity_url + '/' + path;

    let response = await axios.post(url, transferUpdateMsgRequestPayload);

    if (!response.data.updated) {
        throw new Error(`Transfer update failed`);
    }

    storageManager.updateBackupTransactions(walletName, coin.statechain_id, backupTxs);

    let utxo = `${coin.utxo_txid}:${coin.input_vout}`;

    let activity = {
        utxo: utxo,
        amount: coin_amount,
        action: "Transfer",
        date: new Date().toISOString()
    };

    wallet.activities.push(activity);
    coin.status = CoinStatus.IN_TRANSFER;

    storageManager.updateWallet(wallet);

    return coin;
};

const get_new_x1 = async (clientConfig, statechain_id, signed_statechain_id, new_auth_pubkey, batchId) => {

    const statechain_entity_url = clientConfig.statechainEntity;
    const path = "transfer/sender";
    const url = statechain_entity_url + '/' + path;

    let transferSenderRequestPayload = {
        statechain_id: statechain_id,
        auth_sig: signed_statechain_id,
        new_user_auth_key: new_auth_pubkey,
        batch_id: batchId,
    };

    const response = await axios.post(url, transferSenderRequestPayload);

    return response.data.x1;
}

export default { execute };