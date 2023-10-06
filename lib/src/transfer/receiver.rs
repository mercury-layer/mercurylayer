use serde::{Serialize, Deserialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct TransferReceiverRequestPayload { 
    pub statechain_id: String,
    pub batch_data: Option<String>,
    pub t2: String,
    pub auth_sig: String,
}
