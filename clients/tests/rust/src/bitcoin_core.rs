use std::{process::Command, thread};
use anyhow::{anyhow, Result, Ok};

pub fn get_container_id() -> Result<String> {
    // First, get the container ID by running the docker ps command
    let output = Command::new("docker")
        .arg("ps")
        .arg("-qf")
        // .arg("name=lnd_docker-bitcoind-1")
        .arg("name=esplora-container")
        .output()
        .expect("Failed to execute docker ps command");

    // Convert the output to a string and trim whitespace
    let container_id = String::from_utf8_lossy(&output.stdout).trim().to_string();

    if container_id.is_empty() {
        return Err(anyhow!("No container found with the name esplora-container"));
    }

    Ok(container_id)
}

pub fn execute_bitcoin_command(bitcoin_command: &str) -> Result<String> {
    let container_id = get_container_id()?;

    let output = Command::new("docker")
        .arg("exec")
        .arg(&container_id)
        .arg("sh")
        .arg("-c")
        .arg(bitcoin_command)
        .output()
        .expect("Failed to execute docker exec command");

    if output.status.success() {
        return Ok(String::from_utf8_lossy(&output.stdout).to_string().trim().to_string());
    } else {
        return Err(anyhow!("Command execution failed:\n{}", String::from_utf8_lossy(&output.stderr)));
    }
}

pub fn sendtoaddress(amount_in_sats: u32, address: &str) -> Result<String> {

    let amount = amount_in_sats as f64 / 100_000_000.0;

    let bitcoin_command = format!(
        "cli sendtoaddress {} {}", address, amount
    );

    execute_bitcoin_command(&bitcoin_command)
}

pub fn generatetoaddress(num_blocks: u32, address: &str) -> Result<String> {

    let bitcoin_command = format!(
        "cli generatetoaddress {} {}", num_blocks, address
    );

    let res = execute_bitcoin_command(&bitcoin_command);

    // The command may take some time to execute
    thread::sleep(std::time::Duration::from_secs(1));

    res
}

pub fn getnewaddress() -> Result<String> {

    let bitcoin_command = format!(
        "cli getnewaddress"
    );

    execute_bitcoin_command(&bitcoin_command)
}