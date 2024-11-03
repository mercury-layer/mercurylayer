import axios from 'axios';
import initWasm from 'mercury-wasm';
import wasmUrl from 'mercury-wasm/mercury_wasm_bg.wasm?url'
import * as mercury_wasm from 'mercury-wasm';

const infoConfig = async (clientConfig) => {

    let response = await axios.get(`${clientConfig.esploraServer}/api/fee-estimates`);

    let feeRateSatsPerByte = response.data[3];

    if (!feeRateSatsPerByte) {
        feeRateSatsPerByte = 1;
    }

    // console.log(`feeRateSatsPerByte: ${feeRateSatsPerByte}`)

    /* let response = await axios.get(`${clientConfig.esploraServer}/api/v1/fees/recommended`);
    
    const feeRateSatsPerByte = parseInt(response.data.fastestFee, 10);*/

    response = await axios.get(`${clientConfig.statechainEntity}/info/config`);

    return {    
        initlock: response.data.initlock,
        interval: response.data.interval,
        feeRateSatsPerByte,
    };

}

const createActivity = (utxo, amount, action) => {

    const activity = {
        utxo,
        amount,
        action,
        date: new Date().toISOString()
    };

    return activity;
}

const completeWithdraw = async (clientConfig, statechainId, signedStatechainId) => {

    const url = `${clientConfig.statechainEntity}/withdraw/complete`;

    let deleteStatechainPayload = {
        statechain_id: statechainId,
        signed_statechain_id: signedStatechainId
    };

    await axios.post(url, deleteStatechainPayload);
}

const getStatechainInfo = async (clientConfig, statechainId) => {

    const statechainEntityUrl = clientConfig.statechainEntity;
    const path = `info/statechain/${statechainId}`;

    try {
        let response = await axios.get(statechainEntityUrl + '/' + path);
        return response.data;
    } catch (error) {
        if (error.response.status == 404) {
            return null;
        } else {
            throw error;
        }
    }
}

async function getPreviousOutpoint(backupTx) {
    await initWasm(wasmUrl);
    return mercury_wasm.getPreviousOutpoint(backupTx);
}

async function getBlockheight(backupTx) {
    await initWasm(wasmUrl);
    return mercury_wasm.getBlockheight(backupTx);
}

export default { infoConfig, createActivity, completeWithdraw, getStatechainInfo, getPreviousOutpoint, getBlockheight };