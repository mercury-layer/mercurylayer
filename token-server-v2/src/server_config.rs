use config::{Config as ConfigRs, Environment, File};
use serde::{Serialize, Deserialize};
use sqlx::postgres::PgConnectOptions;
use std::{env, fs};

/// Config struct storing all StataChain Entity config
#[derive(Debug, Serialize, Deserialize)]
pub struct ServerConfig {
    /// Public key descriptor for onchain addresses
    pub public_key_descriptor: String,
    /// Bitcoin network
    pub network: String,
    /// Database user
    pub db_user: String,
    /// Database password
    pub db_password: String,
    /// Database host
    pub db_host: String,
    /// Database port
    pub db_port: u16,
    /// Database name
    pub db_name: String,
}

impl Default for ServerConfig {
    fn default() -> ServerConfig {
        ServerConfig {
            public_key_descriptor: String::from(""),
            network: String::from("regtest"),
            db_user: String::from("postgres"),
            db_password: String::from("postgres"),
            db_host: String::from("db_server"),
            db_port: 5432,
            db_name: String::from("mercury"),
        }
    }
}

impl ServerConfig {
    pub fn load() -> Self {
        let mut conf_rs = ConfigRs::default();
        let _ = conf_rs
            // First merge struct default config
            .merge(ConfigRs::try_from(&ServerConfig::default()).unwrap());
        // Override with settings in file Settings.toml if exists
        conf_rs.merge(File::with_name("Settings").required(false));
        // Override with settings in file Rocket.toml if exists
        conf_rs.merge(File::with_name("Rocket").required(false));

        let settings = ConfigRs::builder()
            .add_source(File::with_name("Settings"))
            .build()
            .unwrap();
        
        // Function to fetch a setting from the environment or fallback to the config file
        let get_env_or_config = |key: &str, env_var: &str| -> String {
            env::var(env_var).unwrap_or_else(|_| settings.get_string(key).unwrap())
        };

        ServerConfig {
            db_user: get_env_or_config("db_user", "DB_USER"),
            db_password: get_env_or_config("db_password", "DB_PASSWORD"),
            db_host: get_env_or_config("db_host", "DB_HOST"),
            db_port: get_env_or_config("db_port", "DB_PORT").parse::<u16>().unwrap(),
            db_name: get_env_or_config("db_name", "DB_NAME"),
            public_key_descriptor: get_env_or_config("public_key_descriptor", "PUBLIC_KEY_DESCRIPTOR"),
            network: get_env_or_config("network", "BITCOIN_NETWORK"),
        }
    }

    pub fn build_postgres_connection_string(&self) -> PgConnectOptions {
        PgConnectOptions::new()
            .host(&self.db_host)
            .username(&self.db_user)
            .password(&self.db_password)
            .port(self.db_port)
            .database(&self.db_name)
    }
}