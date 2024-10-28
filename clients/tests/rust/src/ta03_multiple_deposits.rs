use std::{env, process::Command, thread, time::Duration};

use anyhow::{Result, Ok};
use mercuryrustlib::{client_config::ClientConfig, BackupTx, CoinStatus, Wallet};

use crate::{bitcoin_core, electrs};

async fn deposit(amount_in_sats: u32, client_config: &ClientConfig, deposit_address: &str) -> Result<()> {

    let _ = bitcoin_core::sendtoaddress(amount_in_sats, &deposit_address)?;

    let core_wallet_address = bitcoin_core::getnewaddress()?;
    let remaining_blocks = client_config.confirmation_target;
    let _ = bitcoin_core::generatetoaddress(remaining_blocks, &core_wallet_address)?;

    // It appears that Electrs takes a few seconds to index the transaction
    let mut is_tx_indexed = false;

    while !is_tx_indexed {
        is_tx_indexed = electrs::check_address(client_config, &deposit_address, 1000).await?;
        thread::sleep(Duration::from_secs(1));
    }

    Ok(())
}

fn validate_backup_transactions(backup_transactions: &Vec<BackupTx>, interval: u32) -> Result<()> {
    let mut current_txid: Option<String> = None;
    let mut current_vout: Option<u32> = None;
    let mut current_tx_n = 0u32;
    let mut previous_lock_time = 0u32;

    for backup_tx in backup_transactions {
        let outpoint = mercuryrustlib::get_previous_outpoint(&backup_tx)?;

        let current_lock_time = mercuryrustlib::get_blockheight(&backup_tx)?;

        if current_txid.is_some() && current_vout.is_some()  {
            assert!(current_txid.unwrap() == outpoint.txid && current_vout.unwrap() == outpoint.vout);
            assert!(current_lock_time == previous_lock_time - interval);
            assert!(backup_tx.tx_n > current_tx_n);
        } 

        current_txid = Some(outpoint.txid);
        current_vout = Some(outpoint.vout);
        current_tx_n = backup_tx.tx_n;
        previous_lock_time = current_lock_time;
    }

    Ok(())
}

async fn basic_workflow(client_config: &ClientConfig, wallet1: &Wallet, wallet2: &Wallet)  -> Result<()> {

    let amount = 1000;

    let token_id = mercuryrustlib::deposit::get_token(client_config).await?;

    let deposit_address = mercuryrustlib::deposit::get_deposit_bitcoin_address(&client_config, &wallet1.name, &token_id, amount).await?;

    deposit(amount, &client_config, &deposit_address).await?;

    let amount = 2000;

    deposit(amount, &client_config, &deposit_address).await?;

    deposit(amount, &client_config, &deposit_address).await?;

    let amount = 1000;

    deposit(amount, &client_config, &deposit_address).await?;

    mercuryrustlib::coin_status::update_coins(&client_config, &wallet1.name).await?;
    let wallet1: mercuryrustlib::Wallet = mercuryrustlib::sqlite_manager::get_wallet(&client_config.pool, &wallet1.name).await?;

    let new_coin = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 0 && coin.status == CoinStatus::CONFIRMED);
    let duplicated_coin_1 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 1 && coin.status == CoinStatus::DUPLICATED);
    let duplicated_coin_2 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 2 && coin.status == CoinStatus::DUPLICATED);
    let duplicated_coin_3 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 3 && coin.status == CoinStatus::DUPLICATED);

    assert!(new_coin.is_some());
    assert!(duplicated_coin_1.is_some());
    assert!(duplicated_coin_2.is_some());
    assert!(duplicated_coin_3.is_some());

    let new_coin = new_coin.unwrap();
    // let duplicated_coin_1 = duplicated_coin_1.unwrap();
    // let duplicated_coin_2 = duplicated_coin_2.unwrap();
    // let duplicated_coin_3 = duplicated_coin_3.unwrap();

    let statechain_id = new_coin.statechain_id.as_ref().unwrap();

    let wallet2_transfer_adress = mercuryrustlib::transfer_receiver::new_transfer_address(&client_config, &wallet2.name).await?;

    let batch_id = None;

    let force_send = true;

    let duplicated_indexes = vec![1, 3];

    let result = mercuryrustlib::transfer_sender::execute(&client_config, &wallet2_transfer_adress, &wallet1.name, &statechain_id, Some(duplicated_indexes), force_send, batch_id).await;

    // result.unwrap();
    assert!(result.is_ok());

    let wallet1 = mercuryrustlib::sqlite_manager::get_wallet(&client_config.pool, &wallet1.name).await?;

    let new_coin = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 0 && coin.status == CoinStatus::IN_TRANSFER);
    let duplicated_coin_1 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 1 && coin.status == CoinStatus::IN_TRANSFER);
    let duplicated_coin_2 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 2 && coin.status == CoinStatus::DUPLICATED);
    let duplicated_coin_3 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 3 && coin.status == CoinStatus::IN_TRANSFER);

    assert!(new_coin.is_some());
    assert!(duplicated_coin_1.is_some());
    assert!(duplicated_coin_2.is_some());
    assert!(duplicated_coin_3.is_some());

    let new_coin = new_coin.unwrap();

    let backup_transactions = mercuryrustlib::sqlite_manager::get_backup_txs(&client_config.pool, &wallet1.name, &statechain_id).await?;

    /* for backup_tx in backup_transactions.iter() {
        let tx_outpoint = mercuryrustlib::get_previous_outpoint(&backup_tx)?;
        println!("statechain_id: {}", statechain_id);
        println!("txid: {} vout: {}", tx_outpoint.txid, tx_outpoint.vout);
        println!("tx_n: {}", backup_tx.tx_n);
        // println!("client_public_nonce: {}", backup_tx.client_public_nonce);
        // println!("server_public_nonce: {}", backup_tx.server_public_nonce);
        // println!("client_public_key: {}", backup_tx.client_public_key);
        // println!("server_public_key: {}", backup_tx.server_public_key);
        // println!("blinding_factor: {}", backup_tx.blinding_factor);
        println!("----------------------");
    } */

    let info_config = mercuryrustlib::utils::info_config(&client_config).await?;

    let split_backup_transactions = mercuryrustlib::transfer_receiver::split_backup_transactions(&backup_transactions);

    for (index, bk_txs) in split_backup_transactions.iter().enumerate() {
        if index == 0 {
            let first_bkp_tx = bk_txs.first().unwrap();
            let first_backup_outpoint = mercuryrustlib::get_previous_outpoint(&first_bkp_tx)?;
            assert!(first_backup_outpoint.txid == new_coin.utxo_txid.clone().unwrap() && first_backup_outpoint.vout == new_coin.utxo_vout.unwrap());
        }
        validate_backup_transactions(&bk_txs, info_config.interval)?;
    }

    /* println!("Grouped Backup Transactions:");

    for backup_txs in grouped_backup_transactions.iter() {
        for backup_tx in backup_txs.iter() {
            let tx_outpoint = mercuryrustlib::get_previous_outpoint(&backup_tx)?;
            println!("statechain_id: {}", statechain_id);
            println!("txid: {} vout: {}", tx_outpoint.txid, tx_outpoint.vout);
            println!("tx_n: {}", backup_tx.tx_n);
            // println!("client_public_nonce: {}", backup_tx.client_public_nonce);
            // println!("server_public_nonce: {}", backup_tx.server_public_nonce);
            // println!("client_public_key: {}", backup_tx.client_public_key);
            // println!("server_public_key: {}", backup_tx.server_public_key);
            // println!("blinding_factor: {}", backup_tx.blinding_factor);
            println!("----------------------");
        }
    } */

    
    

    // for backup_tx in backup_transactions.iter() {
    //     let tx_outpoint = mercuryrustlib::get_previous_outpoint(&backup_tx)?;
    //     println!("txid: {} vout: {}", tx_outpoint.txid, tx_outpoint.vout);
    //     println!("tx_n: {}", backup_tx.tx_n);
    //     println!("client_public_nonce: {}", backup_tx.client_public_nonce);
    //     println!("server_public_nonce: {}", backup_tx.server_public_nonce);
    //     println!("client_public_key: {}", backup_tx.client_public_key);
    //     println!("server_public_key: {}", backup_tx.server_public_key);
    //     println!("blinding_factor: {}", backup_tx.blinding_factor);
    //     println!("----------------------");
    // }

    let transfer_receive_result = mercuryrustlib::transfer_receiver::execute(&client_config, &wallet2.name).await?;

    let received_statechain_ids = transfer_receive_result.received_statechain_ids;

    assert!(received_statechain_ids.contains(&statechain_id.to_string()));
    assert!(received_statechain_ids.len() == 1);

    let wallet2: mercuryrustlib::Wallet = mercuryrustlib::sqlite_manager::get_wallet(&client_config.pool, &wallet2.name).await?;

    let w2_new_coin = wallet2.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 0 && coin.status == CoinStatus::CONFIRMED);
    let w2_duplicated_coin_1 = wallet2.coins.iter().find(|&coin| coin.statechain_id == Some(statechain_id.to_string()) && coin.duplicate_index == 1 && coin.status == CoinStatus::DUPLICATED);
    let w2_duplicated_coin_2 = wallet2.coins.iter().find(|&coin| coin.statechain_id == Some(statechain_id.to_string()) && coin.duplicate_index == 2 && coin.status == CoinStatus::DUPLICATED);

    assert!(w2_new_coin.is_some());
    assert!(w2_duplicated_coin_1.is_some());
    assert!(w2_duplicated_coin_2.is_some());

    /* let mut coins_json = Vec::new();

    for coin in wallet2.coins.iter() {
        let obj = json!({
            "coin.user_pubkey": coin.user_pubkey,
            "coin.aggregated_address": coin.aggregated_address.as_ref().unwrap_or(&"".to_string()),
            "coin.address": coin.address,
            "coin.statechain_id": coin.statechain_id.as_ref().unwrap_or(&"".to_string()),
            "coin.amount": coin.amount.unwrap_or(0),
            "coin.status": coin.status,
            "coin.locktime": coin.locktime.unwrap_or(0),
            "coin.duplicate_index": coin.duplicate_index,
        });

        coins_json.push(obj);
    }

    let coins_json_string = serde_json::to_string_pretty(&coins_json).unwrap();
    println!("{}", coins_json_string); */

    mercuryrustlib::coin_status::update_coins(&client_config, &wallet1.name).await?;
    let wallet1: mercuryrustlib::Wallet = mercuryrustlib::sqlite_manager::get_wallet(&client_config.pool, &wallet1.name).await?;

    let new_coin = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 0 && coin.status == CoinStatus::TRANSFERRED);
    let duplicated_coin_1 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 1 && coin.status == CoinStatus::TRANSFERRED);
    let duplicated_coin_2 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 2 && coin.status == CoinStatus::INVALIDATED);
    let duplicated_coin_3 = wallet1.coins.iter().find(|&coin| coin.aggregated_address == Some(deposit_address.clone()) && coin.duplicate_index == 3 && coin.status == CoinStatus::TRANSFERRED);

    assert!(new_coin.is_some());
    assert!(duplicated_coin_1.is_some());
    assert!(duplicated_coin_2.is_some());
    assert!(duplicated_coin_3.is_some());

    // TODO: DUPLICATED coins should be invalidated after the other with same statechain_id is transferred

    Ok(())
}

pub async fn execute() -> Result<()> {

    let _ = Command::new("rm").arg("wallet.db").arg("wallet.db-shm").arg("wallet.db-wal").output().expect("failed to execute process");

    env::set_var("ML_NETWORK", "regtest");

    let client_config = mercuryrustlib::client_config::load().await;

    let wallet1 = mercuryrustlib::wallet::create_wallet(
        "wallet1", 
        &client_config).await?;

    mercuryrustlib::sqlite_manager::insert_wallet(&client_config.pool, &wallet1).await?;

    let wallet2 = mercuryrustlib::wallet::create_wallet(
        "wallet2", 
        &client_config).await?;

    mercuryrustlib::sqlite_manager::insert_wallet(&client_config.pool, &wallet2).await?;

    basic_workflow(&client_config, &wallet1, &wallet2).await?;

    println!("TA03 - Test \"Multiple Deposits in the Same Adress\" completed successfully");

    Ok(())
}