const { Command } = require('commander');
const program = new Command();

const mercury_wasm = require('mercury-wasm');

const ElectrumCli = require('@mempool/electrum-client');

const deposit = require('./deposit');
const broadcast_backup_tx = require('./broadcast_backup_tx');
const withdraw = require('./withdraw');
const transfer_receive = require('./transfer_receive');
const transfer_send = require('./transfer_send');

const sqlite3 = require('sqlite3').verbose();

const sqlite_manager = require('./sqlite_manager');

const wallet_manager = require('./wallet');

const config = require('config');

async function main() {

  const db = new sqlite3.Database('wallet.db');

  await sqlite_manager.createTables(db);

  // connect to Electrum

  // const urlElectrum = new URL("tcp://signet-electrumx.wakiyamap.dev:50001");
  const urlElectrum = config.get('electrumServer');
  const urlElectrumObject = new URL(urlElectrum);

  const electrumPort = parseInt(urlElectrumObject.port, 10);
  const electrumHostname = urlElectrumObject.hostname;  
  const electrumProtocol = urlElectrumObject.protocol.slice(0, -1); // remove trailing ':'

  // const electrumClient = new ElectrumCli(50001, '127.0.0.1', 'tcp'); // tcp or tls
  const electrumClient = new ElectrumCli(electrumPort, electrumHostname, electrumProtocol); // tcp or tls
  await electrumClient.connect(); // connect(promise)
  
  program
    .name('Statechain nodejs CLI client')
    .description('CLI to test the Statechain nodejs client')
    .version('0.0.1');
  
  program.command('create-wallet')
    .description('Create a new wallet')
    .argument('<name>', 'name of the wallet')
    .action(async (name) => {

      // const electrumEndpoint = "tcp://127.0.0.1:50001";
      const electrumEndpoint = urlElectrum; //"tcp://signet-electrumx.wakiyamap.dev:50001";
      const statechainEntityEndpoint = config.get('statechainEntity'); // "http://127.0.0.1:8000";
      const network = config.get('network'); // "signet";

      let wallet = await wallet_manager.createWallet(name, electrumClient, electrumEndpoint, statechainEntityEndpoint, network);
 
      await sqlite_manager.insertWallet(db, wallet);

      console.log(wallet);
  
      electrumClient.close();
      db.close();
    });

  program.command('deposit')
    .description('Deposit funds into a statecoin')
    .argument('<wallet_name>', 'name of the wallet')
    .argument('<token_id>', 'token id of the deposit')
    .argument('<amount>', 'amount to deposit')
    .action(async (wallet_name, token_id, amount) => {

      const coin = await deposit.execute(electrumClient, db, wallet_name, token_id, amount);

      console.log(coin);

      electrumClient.close();
      db.close();
    });

    program.command('broadcast-backup-transaction')
      .description('Broadcast a backup transaction via CPFP') 
      .argument('<wallet_name>', 'name of the wallet')
      .argument('<statechain_id>', 'statechain id of the coin')
      .argument('<to_address>', 'recipient bitcoin address')
      .option('-f, --fee_rate <fee_rate>', '(optional) fee rate in satoshis per byte')
      .action(async (wallet_name, statechain_id, to_address, options) => {

       let tx_ids = await broadcast_backup_tx.execute(electrumClient, db, wallet_name, statechain_id, to_address, options.fee_rate);

       console.log(tx_ids);

       electrumClient.close();
       db.close();
    });

    program.command('list-statecoins')
      .description("List wallet's statecoins") 
      .argument('<wallet_name>', 'name of the wallet')
      .action(async (wallet_name) => {

        let wallet = await sqlite_manager.getWallet(db, wallet_name);

        let coins = wallet.coins.map(coin => ({
          statechain_id: coin.statechain_id,
          amount: coin.amount,
          status: coin.status,
          adress: coin.address
        }));
        
        console.log(coins);

        electrumClient.close();
        db.close();

    });

    program.command('withdraw')
      .description('Withdraw funds from a statecoin to a BTC address') 
      .argument('<wallet_name>', 'name of the wallet')
      .argument('<statechain_id>', 'statechain id of the coin')
      .argument('<to_address>', 'recipient bitcoin address')
      .option('-f, --fee_rate <fee_rate>', '(optional) fee rate in satoshis per byte')
      .action(async (wallet_name, statechain_id, to_address, options) => {

        const txid = await withdraw.execute(electrumClient, db, wallet_name, statechain_id, to_address, options.fee_rate);

        console.log({
          txid
        });

        electrumClient.close();
        db.close();
      });

    program.command('new-transfer-address')
      .description('New transfer address for a statecoin') 
      .argument('<wallet_name>', 'name of the wallet')
      .action(async (wallet_name) => {

        const addr = await transfer_receive.newTransferAddress(db, wallet_name)
        console.log({transfer_receive: addr});

        electrumClient.close();
        db.close();
    });

    program.command('transfer-send')
      .description('Send the specified statecoin to an SC address') 
      .argument('<wallet_name>', 'name of the wallet')
      .argument('<statechain_id>', 'statechain id of the coin')
      .argument('<to_address>', 'recipient bitcoin address')
      .action(async (wallet_name, statechain_id, to_address, options) => {

        let coin = await transfer_send.execute(electrumClient, db, wallet_name, statechain_id, to_address);

        console.log(coin);

        electrumClient.close();
        db.close();
      });

    program.command('transfer-receive')
      .description('Retrieve coins from server') 
      .argument('<wallet_name>', 'name of the wallet')
      .action(async (wallet_name) => {

        let received_statechain_ids = await transfer_receive.execute(electrumClient, db, wallet_name);

        console.log(received_statechain_ids);

        electrumClient.close();
        db.close();
    });
  
  
  program.parse();

}

(async () => {
  await main();
})();

