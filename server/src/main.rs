mod endpoints;
mod server_config;
mod server;
mod database;

#[macro_use] extern crate rocket;

use std::time::{Duration, SystemTime, UNIX_EPOCH};

use endpoints::utils;
use rocket::{serde::json::{json, Value}, tokio::{self, time::interval}, Request, Response};
use rocket::fairing::{Fairing, Info};
use rocket::http::Header;
use server::StateChainEntity;

use log::error;

#[catch(500)]
fn internal_error(req: &Request) -> Value {
    let message = format!("500 - Internal Server Error: {}", req.uri());
    error!("{}", message);
    json!(message)
}

#[catch(400)]
fn bad_request(req: &Request) -> Value {
    let message = format!("400 - Bad request: {}", req.uri());
    error!("{}", message);
    json!(message)
}

#[catch(404)]
fn not_found(req: &Request) -> Value {
    let message = format!("404 - Not Found: {}", req.uri());
    error!("{}", message);
    json!(message)
}

async fn broadcast_nip_100(nostr_info: &server_config::NostrInfo) -> Result<(), Box<dyn std::error::Error>> {
    println!("Periodic task: Hello, world!");

    let relay_interval = nostr_info.relay_interval;
    let relay_server = &nostr_info.relay_server;
    let nostr_privkey = &nostr_info.nostr_privkey;

    println!("Relay interval: {}", relay_interval);
    println!("Relay server: {}", relay_server);
    println!("Nostr privkey: {}", nostr_privkey);

    let sec_key = nostr_sdk::Keys::parse(nostr_privkey)?;

    let content = "Mercury server descritpion";
    let tags = vec![
        nostr_sdk::Tag::custom(nostr_sdk::TagKind::Custom("url".into()), ["http://mercury_server.xyz"]),
        nostr_sdk::Tag::custom(nostr_sdk::TagKind::Custom("published_at".into()), ["1296962229"]),
        nostr_sdk::Tag::custom(nostr_sdk::TagKind::Custom("timelock".into()), ["10000"]),
        nostr_sdk::Tag::custom(nostr_sdk::TagKind::Custom("location".into()), ["UK"]),
        nostr_sdk::Tag::custom(nostr_sdk::TagKind::Custom("fee".into()), ["0.0001", "BTC", "true", "false"]),
        nostr_sdk::Tag::custom(nostr_sdk::TagKind::Custom("status".into()), ["active"]),
    ];

    let client = nostr_sdk::Client::new(sec_key.clone());

    client.add_relay(relay_server).await?;

    client.connect().await;

    let created_at = nostr_sdk::Timestamp::now();

    let event = nostr_sdk::EventBuilder::new(
        nostr_sdk::Kind::Custom(39101),  // Custom event kind
        content.to_string(),  // Content
    )
        .tags(tags)
        .custom_created_at(created_at)
        .sign_with_keys(&sec_key)?;

        /* println!("Event as JSON:");
        println!("{}", serde_json::to_string_pretty(&event)?); */


    client.send_event(event).await?;

    Ok(())

}

#[rocket::main]
async fn main() {

    env_logger::init();

    let config = server_config::ServerConfig::load();

    let statechain_entity = StateChainEntity::new().await;

    sqlx::migrate!("./migrations")
        .run(&statechain_entity.pool)
        .await
        .unwrap();

    if config.nostr_info.is_some() {
        let nostr_info = config.nostr_info.unwrap();

        let interval_seconds = nostr_info.relay_interval as u64;

        tokio::spawn(async move {
            let mut ticker = interval(Duration::from_secs(interval_seconds));
            loop {
                ticker.tick().await;
                broadcast_nip_100(&nostr_info).await.unwrap();
            }
        });
    } else {
        println!("No Nostr info found in config file");
    }
    

    let _ = rocket::build()
        .mount("/", routes![
            endpoints::deposit::post_deposit,
            endpoints::deposit::get_token,
            endpoints::deposit::token_init,
            endpoints::sign::sign_first,
            endpoints::sign::sign_second,
            endpoints::lightning_latch::get_paymenthash,
            endpoints::lightning_latch::post_paymenthash,
            endpoints::lightning_latch::transfer_preimage,
            endpoints::transfer_sender::transfer_sender,
            endpoints::transfer_sender::transfer_update_msg,
            endpoints::transfer_receiver::get_msg_addr,
            endpoints::transfer_receiver::statechain_info,
            endpoints::transfer_receiver::transfer_unlock,
            endpoints::transfer_receiver::transfer_receiver,
            endpoints::withdraw::withdraw_complete,
            utils::info_config,
            utils::info_keylist,
            all_options,
        ])
        .register("/", catchers![
            not_found,
            internal_error, 
            bad_request,
        ])
        .manage(statechain_entity)
        .attach(Cors)
        // .attach(MercuryPgDatabase::fairing())
        .launch()
        .await;
}


/// Catches all OPTION requests in order to get the CORS related Fairing triggered.
#[options("/<_..>")]
fn all_options() {
    /* Intentionally left empty */
}

pub struct Cors;

#[rocket::async_trait]
impl Fairing for Cors {
    fn info(&self) -> Info {
        Info {
            name: "Cross-Origin-Resource-Sharing Fairing",
            kind: rocket::fairing::Kind::Response,
        }
    }

    async fn on_response<'r>(&self, _request: &'r Request<'_>, response: &mut Response<'r>) {
        response.set_header(Header::new("Access-Control-Allow-Origin", "*"));
        response.set_header(Header::new(
            "Access-Control-Allow-Methods",
            "POST, PATCH, PUT, DELETE, HEAD, OPTIONS, GET",
        ));
        response.set_header(Header::new("Access-Control-Allow-Headers", "*"));
        response.set_header(Header::new("Access-Control-Allow-Credentials", "true"));
    }
}
