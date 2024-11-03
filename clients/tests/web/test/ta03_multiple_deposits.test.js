import { describe, test, expect } from "vitest";

import axios from 'axios';
import CoinStatus from 'mercuryweblib/coin_enum.js';
import clientConfig from '../ClientConfig.js';
import mercuryweblib from 'mercuryweblib';
import { generateBlocks, depositCoin } from '../test-utils.js';

async function validateBackupTransactions(backupTransactions, interval) {
    let currentTxid = null;
    let currentVout = null;
    let currentTxN = 0;
    let previousLockTime = 0;

    try {
        for (const backupTx of backupTransactions) {
            // Assuming these are async functions that return promises
            const outpoint = await mercuryweblib.getPreviousOutpoint(backupTx);
            const currentLockTime = await mercuryweblib.getBlockheight(backupTx);

            if (currentTxid !== null && currentVout !== null) {
                // Validate transaction chain
                expect(currentTxid).to.equal(outpoint.txid);
                expect(currentVout).to.equal(outpoint.vout);
                expect(currentLockTime).to.equal(previousLockTime - interval);
                expect(backupTx.tx_n).to.be.greaterThan(currentTxN);
            }

            // Update current values
            currentTxid = outpoint.txid;
            currentVout = outpoint.vout;
            currentTxN = backupTx.tx_n;
            previousLockTime = currentLockTime;
        }

        return true; // Success case

    } catch (error) {
        throw error; // Re-throw the error for handling by the caller
    }
}

async function checkDepositsConfirmation(address) {
    let allDepositsConfirmed = false;
    const startTime = Date.now();
    const timeoutDuration = 60000; // 1 minute in milliseconds
    
    while (Date.now() - startTime < timeoutDuration) {

        const response = await axios.get(`${clientConfig.esploraServer}/api/address/${address}/utxo`);
        const transactions = response.data;

        // Check if all transactions are confirmed
        const allConfirmed = transactions.every(tx => tx.status.confirmed === true);
        
        if (allConfirmed) {
            allDepositsConfirmed = true;
            console.log('All deposits confirmed!');
            break;
        }
        
        // Wait for 2 seconds before next check
        await new Promise(resolve => setTimeout(resolve, 2000));
    }
    
    return allDepositsConfirmed;
}

describe('TA03 - Multiple Deposits', () => {
    test("basic workflow", async () => {

        localStorage.removeItem("mercury-layer:wallet1_tb02_1");
        localStorage.removeItem("mercury-layer:wallet2_tb02_1");

        let wallet1 = await mercuryweblib.createWallet(clientConfig, "wallet1_tb02_1");
        let wallet2 = await mercuryweblib.createWallet(clientConfig, "wallet2_tb02_1");

        await mercuryweblib.newToken(clientConfig, wallet1.name);

        let amount = 1000;
        
        let result = await mercuryweblib.getDepositBitcoinAddress(clientConfig, wallet1.name, amount);

        const statechainId = result.statechain_id;
    
        let isDepositInMempool = false;
        let isDepositConfirmed = false;
        let areBlocksGenerated = false;

        await depositCoin(result.deposit_address, amount);

        while (!isDepositConfirmed) {

            const coins = await mercuryweblib.listStatecoins(clientConfig, wallet1.name);
    
            for (let coin of coins) {
                if (coin.statechain_id === statechainId && coin.status === CoinStatus.IN_MEMPOOL && !isDepositInMempool) {
                    isDepositInMempool = true;
                } else if (coin.statechain_id === statechainId && coin.status === CoinStatus.CONFIRMED) {
                    isDepositConfirmed = true;
                    break;
                }
            }

            if (isDepositInMempool && !areBlocksGenerated) {
                areBlocksGenerated = true;
                await generateBlocks(clientConfig.confirmationTarget);
            }
            
            await new Promise(r => setTimeout(r, 1000));
        }

        amount = 2000;

        await depositCoin(result.deposit_address, amount);

        while (true) {
            let coins = await mercuryweblib.listStatecoins(clientConfig, wallet1.name);
            let duplicatedCoin = coins.find(coin => coin.statechain_id === statechainId && coin.status === CoinStatus.DUPLICATED && coin.duplicate_index == 1);
            if (duplicatedCoin) {
                break;
            }
            await new Promise(r => setTimeout(r, 1000));
        }

        amount = 3000;

        await depositCoin(result.deposit_address, amount);

        while (true) {
            let coins = await mercuryweblib.listStatecoins(clientConfig, wallet1.name);
            let duplicatedCoin = coins.find(coin => coin.statechain_id === statechainId && coin.status === CoinStatus.DUPLICATED && coin.duplicate_index == 2);
            if (duplicatedCoin) {
                break;
            }
            await new Promise(r => setTimeout(r, 1000));
        }

        amount = 3000;

        await depositCoin(result.deposit_address, amount);

        while (true) {
            let coins = await mercuryweblib.listStatecoins(clientConfig, wallet1.name);
            let duplicatedCoin = coins.find(coin => coin.statechain_id === statechainId && coin.status === CoinStatus.DUPLICATED && coin.duplicate_index == 3);
            if (duplicatedCoin) {
                break;
            }
            await new Promise(r => setTimeout(r, 1000));
        }

        let coins = await mercuryweblib.listStatecoins(clientConfig, wallet1.name);

        const newCoin = coins.find(coin => 
            coin.statechain_id === statechainId && 
            coin.status == CoinStatus.CONFIRMED && 
            coin.duplicate_index == 0
        );
        
        const duplicatedCoin1 = coins.find(coin => 
            coin.statechain_id === statechainId && 
            coin.status == CoinStatus.DUPLICATED && 
            coin.duplicate_index == 1
        );

        const duplicatedCoin2 = coins.find(coin => 
            coin.statechain_id === statechainId && 
            coin.status == CoinStatus.DUPLICATED && 
            coin.duplicate_index == 2
        );

        const duplicatedCoin3 = coins.find(coin => 
            coin.statechain_id === statechainId && 
            coin.status == CoinStatus.DUPLICATED && 
            coin.duplicate_index == 3
        );

        expect(newCoin).to.not.be.undefined;
        expect(duplicatedCoin1).to.not.be.undefined;
        expect(duplicatedCoin2).to.not.be.undefined;
        expect(duplicatedCoin3).to.not.be.undefined;

        await generateBlocks(clientConfig.confirmationTarget);

        const allDepositsConfirmed = await checkDepositsConfirmation(newCoin.aggregated_address);

        expect(allDepositsConfirmed).to.be.true;

        let transferAddress = await mercuryweblib.newTransferAddress(wallet2.name);

        let duplicatedIndexes = [1, 3];

        result = await mercuryweblib.transferSend(
            clientConfig,
            wallet1.name,
            statechainId,
            transferAddress.transfer_receive,
            true, 
            null,            
            duplicatedIndexes
        );
        expect(result).to.have.property('statechain_id');

        const jsonData = localStorage.getItem(`mercury-layer:${wallet1.name}-${statechainId}`);
        let backupTxs = JSON.parse(jsonData);

        expect(backupTxs.length).to.equal(6);

        const split_backup_transactions = mercuryweblib.splitBackupTransactions(backupTxs);

        const infoConfig = await mercuryweblib.infoConfig(clientConfig);

        for (const [index, bkTxs] of split_backup_transactions.entries()) {
            if (index === 0) {
                const firstBkpTx = bkTxs[0];
                expect(firstBkpTx).to.not.be.undefined;

                const firstBackupOutpoint = await mercuryweblib.getPreviousOutpoint(firstBkpTx);
                expect(firstBackupOutpoint).to.not.be.undefined;
                expect(newCoin.utxo_txid).to.be.equals(firstBackupOutpoint.txid);
                expect(newCoin.utxo_vout).to.be.equals(firstBackupOutpoint.vout);
            }

            await validateBackupTransactions(bkTxs, infoConfig.interval);
        }

        let transferReceiveResult = await mercuryweblib.transferReceive(clientConfig, wallet2.name);
        expect(transferReceiveResult.receivedStatechainIds).contains(newCoin.statechain_id);
        expect(transferReceiveResult.receivedStatechainIds.length).to.equal(1);

        

    });
}, 500000);
