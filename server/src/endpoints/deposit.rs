use std::str::FromStr;

use bitcoin::hashes::{sha256, Hash};
use rocket::{serde::json::Json, response::status, State, http::Status};
use secp256k1_zkp::{XOnlyPublicKey, schnorr::Signature, Message, Secp256k1, PublicKey};
use serde::{Serialize, Deserialize};
use serde_json::{Value, json};
use crate::{server::StateChainEntity, server_config::Enclave};

pub async fn get_token_no_server(statechain_entity: &State<StateChainEntity>, config: &crate::server_config::ServerConfig) -> status::Custom<Json<Value>>  {

    if config.network == "mainnet" {
        let response_body = json!({
            "error": "Internal Server Error",
            "message": "Token generation not supported on mainnet."
        });
    
        return status::Custom(Status::InternalServerError, Json(response_body));
    }

    let token_id = uuid::Uuid::new_v4().to_string();   

    crate::database::deposit::insert_new_token(&statechain_entity.pool, &token_id).await;

    let token = mercurylib::deposit::TokenResponse {
        token_id,
        payment_method: "free".to_string(),
        deposit_address: None,
        fee: 0,
        confirmation_target: 0,
    };

    let response_body = json!(token);

    return status::Custom(Status::Ok, Json(response_body));
}

pub async fn get_token_from_server(config: &crate::server_config::ServerConfig) -> status::Custom<Json<Value>>  {

    let client: reqwest::Client = reqwest::Client::new();
    let request = client.get(&format!("{}/token/token_gen", config.token_server_url.as_ref().unwrap()));

    let value = match request.send().await {
        Ok(response) => {
            let text = response.text().await.unwrap();
            text
        },
        Err(err) => {
            let response_body = json!({
                "message": err.to_string()
            });

            let err = err.status();
            let status = if err.is_some() {
                Status::from_code(err.unwrap().as_u16()).unwrap_or(Status::InternalServerError)
            } else {
                Status::InternalServerError
            };

        
            return status::Custom(status, Json(response_body));
        },
    };

    let response: serde_json::Value = serde_json::from_str(value.as_str()).expect(&format!("failed to parse: {}", value.as_str()));

    let token_id = response.get("token_id").unwrap().as_str().unwrap().to_string();
    let deposit_address = response.get("deposit_address").unwrap().as_str().unwrap().to_string();
    let fee = response.get("fee").unwrap().as_u64().unwrap();
    let confirmation_target = response.get("confirmation_target").unwrap().as_u64().unwrap();

    let token = mercurylib::deposit::TokenResponse {
        token_id,
        payment_method: "onchain".to_string(),
        deposit_address: Some(deposit_address),
        fee,
        confirmation_target,
    };

    let response_body = json!(token);

    return status::Custom(Status::Ok, Json(response_body));
}

#[get("/deposit/get_token")]
pub async fn get_token(statechain_entity: &State<StateChainEntity>) -> status::Custom<Json<Value>>  {

    let config = crate::server_config::ServerConfig::load();

    if config.token_server_url.is_none() {
        return get_token_no_server(statechain_entity, &config).await;
    } else {
        return get_token_from_server(&config).await;
    }
}

/* #[get("/tokens/token_init")]
pub async fn token_init(statechain_entity: &State<StateChainEntity>) -> status::Custom<Json<Value>>  {

    let config = crate::server_config::ServerConfig::load();

    if config.network == "mainnet" {
        let response_body = json!({
            "error": "Internal Server Error",
            "message": "Token generation not supported on mainnet."
        });
    
        return status::Custom(Status::InternalServerError, Json(response_body));
    }

    let btc_payment_address = String::from("tb1qdgjdmmsdp5hkrhwl6cxd3uvt6hvjvlmmzucdca");
    let fee =  String::from("0.0001");
    let lightning_invoice =  String::from("lnbc10u1pj3knpdsp5k9f25s2wpzewkf9c78pftkgnkuuz82erkcjml7zkgsp7znyhs5yspp5rxz3tkc7ydgln3u7ez6duhp0g6jpzgtnn7ph5xrjy6muh9xm07wqdp2f9h8vmmfvdjjqen0wgsy6ctfdeehgcteyp6x76m9dcxqyjw5qcqpj9qyysgq6z9whs8am75r6mzcgt76vlwgk5g9yq5g8xefdxx6few6d5why7fs7h5g2dx9hk7s60ywtnkyc0f3p0cha4a9kmgkq5jvu5e7hvsaawqpjtf8p4");
    let processor_id = uuid::Uuid::new_v4().to_string();
    let token_id = uuid::Uuid::new_v4().to_string();
    let confirmed = false;
    let spent = false;
    let expiry = String::from("2024-12-26T17:29:50.013Z");

    crate::database::deposit::insert_new_token(&statechain_entity.pool, &token_id).await;

    let token = mercurylib::wallet::Token {
        btc_payment_address,
        fee,
        lightning_invoice,
        processor_id,
        token_id,
        confirmed,
        spent,
        expiry
    };

    let response_body = json!(token);

    return status::Custom(Status::Ok, Json(response_body));
} */

fn get_random_enclave_index(statechain_id: &str, enclaves: &Vec<Enclave>) -> Result<usize, String> {
    let index_from_statechain_id = get_enclave_index_from_statechain_id(statechain_id, enclaves.len() as u32);

    let selected_enclave = enclaves.get(index_from_statechain_id).unwrap();
    if selected_enclave.allow_deposit {
        return Ok(index_from_statechain_id);
    } else {
        for (i, enclave) in enclaves.iter().enumerate() {
            if enclave.allow_deposit {
                return Ok(i);
            }
        }
    }

    Err("No valid enclave found with allow_deposit set to true".to_string())
}

fn get_enclave_index_from_statechain_id(statechain_id: &str, enclave_array_len: u32) -> usize {
    let hash = sha256::Hash::hash(statechain_id.as_bytes());
    let hash_bytes = hash.as_byte_array();
    let mut bytes = [0u8; 16];
    bytes.copy_from_slice(&hash_bytes[..16]);
    let random_number = u128::from_be_bytes(bytes);

    return (random_number % enclave_array_len as u128) as usize;
}

struct TokenStatusResponse {
    confirmed: bool,
    spent: bool,
    err: bool,
    status: Option<Status>,
    err_message: Option<String>
}

pub async fn check_token_status(token_id: &str) -> TokenStatusResponse{

    let config = crate::server_config::ServerConfig::load();

    let client: reqwest::Client = reqwest::Client::new();
    let request = client.get(&format!("{}/token/token_verify/{}", config.token_server_url.as_ref().unwrap(), token_id));

    let value = match request.send().await {
        Ok(response) => {
            let text = response.text().await.unwrap();
            text
        },
        Err(err) => {
            let message = err.to_string();

            let err = err.status();
            let status = if err.is_some() {
                Status::from_code(err.unwrap().as_u16()).unwrap_or(Status::InternalServerError)
            } else {
                Status::InternalServerError
            };

            return TokenStatusResponse {
                confirmed: false,
                spent: false,
                err: true,
                status: Some(status),
                err_message: Some(message),
            };
        },
    };

    let response: serde_json::Value = serde_json::from_str(value.as_str()).expect(&format!("failed to parse: {}", value.as_str()));

    let confirmed = response.get("confirmed").unwrap().as_bool().unwrap();
    let spent = response.get("spent").unwrap().as_bool().unwrap();

    return TokenStatusResponse {
        confirmed,
        spent,
        err: false,
        status: None,
        err_message: None,
    };
}

#[post("/deposit/init/pod", format = "json", data = "<deposit_msg1>")]
pub async fn post_deposit(statechain_entity: &State<StateChainEntity>, deposit_msg1: Json<mercurylib::deposit::DepositMsg1>) -> status::Custom<Json<Value>> {

    let statechain_entity = statechain_entity.inner();

    let auth_key = XOnlyPublicKey::from_str(&deposit_msg1.auth_key).unwrap();
    let token_id = deposit_msg1.token_id.clone();
    let signed_token_id = Signature::from_str(&deposit_msg1.signed_token_id.to_string()).unwrap();

    let msg = Message::from_hashed_data::<sha256::Hash>(token_id.to_string().as_bytes());

    let secp = Secp256k1::new();
    if !secp.verify_schnorr(&signed_token_id, &msg, &auth_key).is_ok() {

        let response_body = json!({
            "message": "Signature does not match authentication key."
        });
    
        return status::Custom(Status::InternalServerError, Json(response_body));

    }

    let is_existing_key = crate::database::deposit::check_existing_key(&statechain_entity.pool, &auth_key).await;

    if is_existing_key {
        let response_body = json!({
            "message": "The authentication key is already assigned to a statecoin."
        });
    
        return status::Custom(Status::BadRequest, Json(response_body));
    }

    /* let valid_token =  crate::database::deposit::get_token_status(&statechain_entity.pool, &token_id).await;

    if valid_token.is_none() {
        let response_body = json!({
            "error": "Deposit Error",
            "message": "Token ID not found."
        });
    
        return status::Custom(Status::NotFound, Json(response_body));
    }

    if !valid_token.unwrap() {
        let response_body = json!({
            "error": "Deposit Error",
            "message": "Token unpaid or used."
        });
    
        return status::Custom(Status::Gone, Json(response_body));
    } */

   let token_info = crate::database::deposit::get_token_info(&statechain_entity.pool, &token_id).await;

   if token_info.is_none() {
        let response_body = json!({
            "error": "Deposit Error",
            "message": "Token ID not found."
        });

        return status::Custom(Status::NotFound, Json(response_body));
    }

    let token_info = token_info.unwrap();

    if token_info.spent {
        let response_body = json!({
            "message": "Token already spent."
        });

        return status::Custom(Status::Gone, Json(response_body));
    }

    if !token_info.confirmed {

        let token_status_response = check_token_status(&token_id).await;

        if token_status_response.err {
            let response_body = json!({
                "message": token_status_response.err_message.unwrap()
            });
        
            return status::Custom(token_status_response.status.unwrap(), Json(response_body));
        }

        if token_status_response.spent {
            let response_body = json!({
                "message": "Token already spent."
            });
    
            return status::Custom(Status::Gone, Json(response_body));
        }

        if !token_status_response.confirmed {
            let response_body = json!({
                "message": "Token not confirmed."
            });
        
            return status::Custom(Status::Gone, Json(response_body));
        }
    }

    #[derive(Debug, Serialize, Deserialize)]
    pub struct GetPublicKeyRequestPayload {
        statechain_id: String,
    }

    let statechain_id = uuid::Uuid::new_v4().as_simple().to_string();

    let config = crate::server_config::ServerConfig::load();

    let enclave_index = get_random_enclave_index(&statechain_id, &config.enclaves).unwrap();

    let lockbox_endpoint = config.enclaves.get(enclave_index).unwrap().url.clone();
    let path = "get_public_key";

    let client: reqwest::Client = reqwest::Client::new();
    let request = client.post(&format!("{}/{}", lockbox_endpoint, path));

    let payload = GetPublicKeyRequestPayload {
        statechain_id: statechain_id.clone(),
    };

    let value = match request.json(&payload).send().await {
        Ok(response) => {
            let text = response.text().await.unwrap();
            text
        },
        Err(err) => {
            let response_body = json!({
                "error": "Internal Server Error",
                "message": err.to_string()
            });
        
            return status::Custom(Status::InternalServerError, Json(response_body));
        },
    };

    #[derive(Serialize, Deserialize)]
    pub struct PublicNonceRequestPayload<'r> {
        server_pubkey: &'r str,
    }

    let response: PublicNonceRequestPayload = serde_json::from_str(value.as_str()).expect(&format!("failed to parse: {}", value.as_str()));

    let mut server_pubkey_hex = response.server_pubkey.to_string();

    if server_pubkey_hex.starts_with("0x") {
        server_pubkey_hex = server_pubkey_hex[2..].to_string();
    }

    let server_pubkey = PublicKey::from_str(&server_pubkey_hex).unwrap();

    crate::database::deposit::insert_new_deposit(&statechain_entity.pool, &token_id, &auth_key, &server_pubkey, &statechain_id, enclave_index as i32).await;

    crate::database::deposit::set_token_spent(&statechain_entity.pool, &token_id).await;

    let deposit_msg1_response = mercurylib::deposit::DepositMsg1Response {
        server_pubkey: server_pubkey.to_string(),
        statechain_id,
    };

    let response_body = json!(deposit_msg1_response);

    status::Custom(Status::Ok, Json(response_body))
}
