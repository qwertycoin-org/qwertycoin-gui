Release Notes 4.0.2
- Add DNS Checkpoints
- Remove obsolete builtin cpu mining 
- Show private keys separately in Export keys dialog
- Improved Translations
- Add Korean, Japanese, Chinese, Indian and Portoguese language
- Bugfixes

Release Notes 4.0.1
Qwertycoin plan his second stage for the upcoming hard fork to heavy algorithm
- New network identifiers to ensure that every single Users, Poolowner, Exchange update their Qwertycoin Software
- Upgrade the core to Cryptonight-Heavy / ASIC Resistant ON HEIGHT 110,520
- Upgrade Blocksize for creating larger Blocks and Transactions
- Increase Syncperformance
- Load Checkpoints from CSV file (CLI Only)
- Better Rpc error code handling
- Unmixable dust update
- Fix Debug for win32 systems
- Check for fee address
- Masternode optional check for fee in relayed transaction
- Checked double bug. ok. this make 2nd key track 
- Added RPC method validateaddress
- New branding

* Wallet doesn't include unmixable dust in transactions; to send dust to yourself use menu Wallet -> Sweep unmixable (there's no point doing this if dust amount is less than miners fee)
* Unmixable dust is displayed in Overview
* 'Send to' option in Address Book context menu (also triggered by double click on contact)
* Minimal mixin in 'Send' now is 1 (if you need to sweep dust use corresponding menu option)
* Updated to latest Qwertycoin core
* Reorganized menu

# IMPORTANT:
## DO NOT use a copy of your old blockchain file as part of this upgrade process!! 

### Seriously, we can’t emphasize this enough.  You won’t save time and will only be disappointed with the results.


#### Step-by-step instructions:

1. Open your old wallet:
   1. Copy your Mnemonic Seed - **File -> Show Mnemonic Seed** (if all else fails, this will be your safety net)
   1. Backup your old wallet - **File -> Backup Wallet** (you will use this file later in this process)
   1. Copy your backup file to a safe place
1. Download and install the new Qwerty Wallet
1. Open the new wallet software
1. You will get an error message, “Error: Socket operation timed out”
   1. This error is expected - we are working on a fix for this bug and will release the fix in a future update
1. A new a new wallet is created for you - don’t panic - if you followed step #1 above your coins are safe
1. Let the wallet automatically synchronize with the new blockchain
   1. This may take an hour or more depending on the speed of your internet connection, the power of your CPU and the load on the Qwertycoin network
1. Once the synchronize operation is complete, open the wallet backup you saved in step #1b above - **File -> Open Wallet**
1. Again, let the wallet synchronize.  This will occur much faster than it did in step #6
1. Pour yourself a drink - congratulations your wallet is upgraded!

We are constantly looking for ways to improve our documentation.  Please post your feedback to the Qwerty Support Telegram channel or to the Qwerty Discord site.


Release Notes 1.8.1
-Added Release Notes
-Increase TX Sizes
-Changes in CI
-Several Changes and Bugfixes (Update Socket Error)

Release Notes 1.0.0
Project move to Github