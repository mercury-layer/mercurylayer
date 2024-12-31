
import axios from 'axios';
import initWasm from 'mercury-wasm';
import wasmUrl from 'mercury-wasm/mercury_wasm_bg.wasm?url'
import * as mercury_wasm from 'mercury-wasm';
import storageManager from './storage_manager.js';
import transaction from './transaction.js';
import utils from './utils.js';

/* const getTokenFromServer = async (clientConfig) => {

    const statechain_entity_url = clientConfig.statechainEntity;
    const path = "tokens/token_init";
    const url = statechain_entity_url + '/' + path;

    const response = await axios.get(url);

    if (response.status != 200) {
        throw new Error(`Token error: ${response.data}`);
    }

    let token = response.data;

    return token;
} */

const getTokenFromServer = async (clientConfig) => {

    const statechain_entity_url = clientConfig.statechainEntity;
    const path = "deposit/get_token";
    const url = statechain_entity_url + '/' + path;

    const response = await axios.get(url);

    if (response.status != 200) {
        throw new Error(`Token error: ${response.data}`);
    }

    let token = response.data;

    return token;
}

const getToken = async (clientConfig, walletName) => {

    let token_response = await getTokenFromServer(clientConfig);

    // for dev purposes
    /* let token = {};
    token.token_id = token_response.token_id;
    token.confirmed = true;
    token.spent = false;
    token.btc_payment_address = "b1qdgjdmmsdp5hkrhwl6cxd3uvt6hvjvlmmzucdca";
    token.fee = "0.0001";
    token.lightning_invoice = "lnbc10u1pj3knpdsp5k9f25s2wpzewkf9c78pftkgnkuuz82erkcjml7zkgsp7znyhs5yspp5rxz3tkc7ydgln3u7ez6duhp0g6jpzgtnn7ph5xrjy6muh9xm07wqdp2f9h8vmmfvdjjqen0wgsy6ctfdeehgcteyp6x76m9dcxqyjw5qcqpj9qyysgq6z9whs8am75r6mzcgt76vlwgk5g9yq5g8xefdxx6few6d5why7fs7h5g2dx9hk7s60ywtnkyc0f3p0cha4a9kmgkq5jvu5e7hvsaawqpjtf8p4";
    token.processor_id = "1";
    token.expiry = "2024-12-26T17:29:50.013Z";

    let wallet = storageManager.getWallet(walletName);

    wallet.tokens.push(token); 

    storageManager.updateWallet(wallet); */

    return token_response;
}

const init = async (clientConfig, wallet, token_id) => {

    let coin = mercury_wasm.getNewCoin(wallet);    

    wallet.coins.push(coin);

    storageManager.updateWallet(wallet);

    let depositMsg1 = mercury_wasm.createDepositMsg1(coin, token_id);

    const statechain_entity_url = clientConfig.statechainEntity;
    const path = "deposit/init/pod";
    const url = statechain_entity_url + '/' + path;

    const response = await axios.post(url, depositMsg1);

    if (response.status != 200) {
        throw new Error(`Deposit error: ${response.data}`);
    }

    let depositMsg1Response = response.data;

    let depositInitResult = mercury_wasm.handleDepositMsg1Response(coin, depositMsg1Response);

    coin.statechain_id = depositInitResult.statechain_id;
    coin.signed_statechain_id = depositInitResult.signed_statechain_id;
    coin.server_pubkey = depositInitResult.server_pubkey;

    storageManager.updateWallet(wallet);
}

const getDepositBitcoinAddress = async (clientConfig, walletName, token_id, amount) => {

    await initWasm(wasmUrl);

    let wallet = storageManager.getWallet(walletName);

    /* let foundToken = wallet.tokens.find(token => token.confirmed === true && token.spent === false);

    if (!foundToken) {
        throw new Error(`There is no token available`);
    }

    await init(clientConfig, wallet, foundToken.token_id); */

    await init(clientConfig, wallet, token_id);

    let coin = wallet.coins[wallet.coins.length - 1];

    let aggregatedPublicKey = mercury_wasm.createAggregatedAddress(coin, wallet.network);

    coin.amount = parseInt(amount, 10);
    coin.aggregated_address = aggregatedPublicKey.aggregate_address;
    coin.aggregated_pubkey = aggregatedPublicKey.aggregate_pubkey;

    // foundToken.spent = true;

    storageManager.updateWallet(wallet);

    return { "deposit_address":  coin.aggregated_address, "statechain_id": coin.statechain_id };
}

const createTx1 = async (clientConfig, coin, walletNetwork, txN) => {

    const toAddress = mercury_wasm.getUserBackupAddress(coin, walletNetwork);
    const isWithdrawal = false;
    const qtBackupTx = 0;

    const serverInfo = await utils.infoConfig(clientConfig);

    let feeRateSatsPerByte = (serverInfo.feeRateSatsPerByte > clientConfig.maxFeeRate) ? clientConfig.maxFeeRate: serverInfo.feeRateSatsPerByte;

    let signedTx = await transaction.newTransaction(
        clientConfig, 
        coin, 
        toAddress, 
        isWithdrawal, 
        qtBackupTx, 
        null, 
        walletNetwork,
        feeRateSatsPerByte,
        serverInfo.initlock,
        serverInfo.interval
    );

    let backupTx = {
        tx_n: txN,
        tx: signedTx,
        client_public_nonce: coin.public_nonce, 
        server_public_nonce: coin.server_public_nonce,
        client_public_key: coin.user_pubkey,
        server_public_key: coin.server_pubkey,
        blinding_factor: coin.blinding_factor
    };

    coin.locktime = mercury_wasm.getBlockheight(backupTx);

    return backupTx;
}

export default { getToken, getDepositBitcoinAddress, createTx1 }
