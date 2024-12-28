mod endpoints;
mod server_config;
mod server_state;

use std::str::FromStr;

use miniscript::{bitcoin::Network, Descriptor, DescriptorPublicKey};

#[macro_use] extern crate rocket;

use rocket::{serde::json::{Value, json}, Request, Response};
use rocket::fairing::{Fairing, Info, Kind};
use rocket::http::Header;
use server_state::TokenServerState;

#[catch(500)]
fn internal_error() -> Value {
    json!("Internal server error")
}

#[catch(400)]
fn bad_request() -> Value {
    json!("Bad request")
}

#[catch(404)]
fn not_found(req: &Request) -> Value {
    json!(format!("Not found! Unknown route '{}'.", req.uri()))
}

#[rocket::main]
async fn main() {

    main2();

    server_config::ServerConfig::load();

    let token_server = TokenServerState::new().await;

    let _ = rocket::build()
        .mount("/", routes![
            /* endpoints::token::token_init,
            endpoints::token::token_verify, */
            endpoints::token::token_gen,
            all_options,
        ])
        .register("/", catchers![
            not_found,
            internal_error, 
            bad_request,
        ])
        .manage(token_server)
        .attach(Cors)
        // .attach(MercuryPgDatabase::fairing())
        .configure(rocket::Config::figment().merge(("port", 8001)))
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
            kind: Kind::Response,
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

fn main2() {

    let desc_str = "wpkh([656a457c/84'/1'/0']tpubDCTXiLu1wcqUwQK6QMPPUTBzbRjsqMABzCvd5vG22KGoA95cTG1VkszQQJyx24UP8KEJVKrKRDRtUPodHVV59CfNqUkXjKUagowHJVSWq4C/0/*)#vn0n5xcd";

    let descriptor = Descriptor::<DescriptorPublicKey>::from_str(desc_str).unwrap();

    let network = Network::Regtest;

    // Get the unique identifier (descriptor without checksum + checksum)
    let desc_id = descriptor.to_string();  // This includes the checksum
    println!("Descriptor ID (with checksum): {}", desc_id);

    // If you need just the checksum part
    if let Some(checksum) = desc_id.split('#').nth(1) {
        println!("Descriptor checksum: {}", checksum);
    }

    // Derive addresses for indices 0 to 5
    for i in 0..5 {

        let derived_desc = descriptor.at_derivation_index(i).unwrap();
        
        // Get the address
        let address = derived_desc.address(network).unwrap();
        
        println!("Index {}: {}", i, address);

    }
}
