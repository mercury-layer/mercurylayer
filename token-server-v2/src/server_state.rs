use std::sync::Mutex;

use sqlx::{Pool, Postgres, postgres::PgPoolOptions};

use crate::server_config::ServerConfig;

pub struct TokenServerState {
    pub config: ServerConfig,
    pub pool: Pool<Postgres>,
    key_index: Mutex<u64>,
}

impl TokenServerState {
    pub async fn new() -> Self {

        let config = ServerConfig::load();
        let connection_string = config.build_postgres_connection_string();
        
        let pool = 
            PgPoolOptions::new()
            // .max_connections(5)
            .connect_with(connection_string)
            .await
            .unwrap();

        let key_index = 0;

        TokenServerState {
            config,
            pool,
            key_index: Mutex::new(key_index),
        }
    }
}
