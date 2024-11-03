const namespace = 'mercury-layer'

const createWallet = (wallet) => {
    setItem(wallet.name, wallet);
}

const updateWallet = (wallet) => {
    setItem(wallet.name, wallet, true);
}

const getWallet = (walletName) => {
    return getItem(walletName);
}

const createBackupTransactions = (walletName, statechainId, backupTransactions) => {
    const key = `${walletName}-${statechainId}`;
    setItem(key, backupTransactions);
}

const updateBackupTransactions = (walletName, statechainId, backupTransactions) => {
    const key = `${walletName}-${statechainId}`;
    setItem(key, backupTransactions, true);
}

const getBackupTransactions = (walletName, statechainId) => {
    const key = `${walletName}-${statechainId}`;
    return getItem(key);
}

const setItem = (key, jsonData, isUpdate = false) => {
    const namespacedKey = `${namespace}:${key}`;
    
    if (!isUpdate && localStorage.getItem(namespacedKey) !== null) {
        throw new Error(`Key "${namespacedKey}" already exists.`);
    }
    
    localStorage.setItem(namespacedKey, JSON.stringify(jsonData));
}

const getItem = (key) => {
    const namespacedKey = `${namespace}:${key}`;
    const jsonData = localStorage.getItem(namespacedKey);
    
    if (jsonData === null) {
        throw new Error(`Key "${namespacedKey}" does not exist.`);
    }
    
    return JSON.parse(jsonData);
}

export default { createWallet, updateWallet, getWallet, createBackupTransactions, updateBackupTransactions, getBackupTransactions };
