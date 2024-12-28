use std::{collections::HashMap, str::FromStr};

use bitcoin::network;
use miniscript::{bitcoin::bech32::primitives::checksum, Descriptor, DescriptorPublicKey};
use rocket::{serde::json::Json, response::status, State, http::Status};
use serde::{Serialize, Deserialize};
use serde_json::{Value, json};
use sqlx::Row;

use crate::{server_config, server_state::TokenServerState};

pub async fn get_descriptor_index(tx: &mut sqlx::Transaction<'_, sqlx::Postgres>, checksum: &str) -> i32 {
    // Lock the table first to prevent concurrent insertions
    sqlx::query("LOCK TABLE public.tokens IN SHARE MODE")
        .execute(&mut **tx)
        .await
        .unwrap();

    // Then get the max index
    let row = sqlx::query(
        "SELECT MAX(onchain_address_index) \
        FROM public.tokens \
        WHERE descriptor_checksum = $1"
    )
    .bind(checksum)
    .fetch_one(&mut **tx)
    .await
    .unwrap();

    let index: Option<i32> = row.get(0);
    index.map(|i| i + 1).unwrap_or(0)
}

pub async fn insert_new_token(
    tx: &mut sqlx::Transaction<'_, sqlx::Postgres>, 
    token_id: &str, 
    onchain_address: &str, 
    descriptor_checksum: &str, 
    onchain_address_index: i32
) {
    let query = "INSERT INTO tokens (token_id, onchain_address, descriptor_checksum, onchain_address_index, confirmed, spent) \
                 VALUES ($1, $2, $3, $4, $5, $6)";
    sqlx::query(query)
        .bind(token_id)
        .bind(onchain_address)
        .bind(descriptor_checksum)
        .bind(onchain_address_index)
        .bind(false)
        .bind(false)
        .execute(&mut **tx)
        .await
        .unwrap();
}

#[get("/token/token_gen")]
pub async fn token_gen(token_server: &State<TokenServerState>) -> status::Custom<Json<Value>> {

    let server_config = server_config::ServerConfig::load();
    let descriptor = Descriptor::<DescriptorPublicKey>::from_str(&server_config.public_key_descriptor).unwrap();
    let network = miniscript::bitcoin::Network::from_str(server_config.network.as_str()).unwrap();
    let desc_str = descriptor.to_string();
    let checksum = desc_str.split('#').nth(1);
    if checksum.is_none() {
        return status::Custom(Status::InternalServerError, Json(json!("Unable to get descriptor checksum")));
    }
    let checksum = checksum.unwrap();

    // Start a transaction
    let mut tx = token_server.pool.begin().await.unwrap();

    // Get next index within transaction
    let index = get_descriptor_index(&mut tx, checksum).await;

    let derived_desc = descriptor.at_derivation_index(index as u32).unwrap();
    let address = derived_desc.address(network).unwrap();
    let token_id = uuid::Uuid::new_v4().to_string();
    let onchain_address = address.to_string();

    // Insert within the same transaction
    insert_new_token(&mut tx, &token_id, &onchain_address, checksum, index).await;

    // Commit the transaction
    tx.commit().await.unwrap();

    let response_body = json!({
        "token_id": token_id,
        "deposit_address": onchain_address,
    });
    
    status::Custom(Status::Ok, Json(response_body))
}
