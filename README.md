![image](https://cdn.qwertycoin.org/images/press/other/qwc-github-3.png)
#### Master Build Status
[![Build Status](https://travis-ci.org/qwertycoin-org/qwertycoin-gui.svg?branch=master)](https://travis-ci.org/qwertycoin-org/qwertycoin-gui)
[![Build status](https://ci.appveyor.com/api/projects/status/udpsj8mf5x7s1rt6/branch/master?svg=true)](https://ci.appveyor.com/project/Qwertycoin/qwertycoin-gui/branch/master)


#### Development Build Status
[![Build Status](https://travis-ci.org/qwertycoin-org/qwertycoin-gui.svg?branch=dev)](https://travis-ci.org/qwertycoin-org/qwertycoin-gui) [![Build status](https://ci.appveyor.com/api/projects/status/udpsj8mf5x7s1rt6/branch/dev?svg=true)](https://ci.appveyor.com/project/Qwertycoin/qwertycoin-gui/branch/dev)

# Table of contents
1. [Project Specs](#coinspecs)
2. [How to Compile Qwertycoin](#howtocompile)
    1. [Qwertycoin for Linux](#build-linux)
    2. [Qwertycoin for Windows](#build-windows)
    3. [Qwertycoin for macOS](#build-apple)
3. [Downloads](#downloads)
4. [Useful Links](#usefullinks)
5. [Donate & Thanks](#donate)
6. [Copyright / License](#copypasta)

## Installing <a name="installing"></a>
We offer binary images of the latest releases here: https://releases.qwertycoin.org

If you would like to compile yourself, read on.

### Coin Specs <a name="coinspecs"></a>
<table>
<tr><td>Ticker Symbol</td><td>QWC</td></tr>
<tr><td>Algorithm</td><td>Cryptonight</td></tr>
<tr><td>Type</td><td>Proof of Work (PoW)</td></tr>
<tr><td>Block Time</td><td>120 Seconds</td></tr>
<tr><td>Premine</td><td>0.0 %</td></tr>
<tr><td>Decimals</td><td>8 Digits</td></tr>
<tr><td>Block Reward</td><td>Decrease by each block</td></tr>
<tr><td>Max Coin Supply </td><td>184,467,440,737 QWC</td></tr>
<tr><td>P2P | RPC Port</td><td>8196 | 8197</td></tr>
</table>

More information at [qwertycoin.org](https://qwertycoin.org/)

# How To Compile <a name="howtocompile"></a>

#### Linux  <a name="build-linux"></a>

##### Prerequisites

- You will need the following packages: build-essential, [cmake (3.10 or higher)](https://github.com/qwertycoin-org/qwertycoin/wiki/E01.-Install-Cmake-3.10) and git;
- Most of these should already be installed on your system. For example on Ubuntu by running:
```
sudo apt-get install build-essential python-dev gcc g++ git cmake libboost-all-dev qtbase5-dev
```

##### Building

- After installing dependencies run simple script:
```
git clone --recurse-submodules https://github.com/qwertycoin-org/qwertycoin-gui
cd ./qwertycoin-gui
mkdir ./build
cd ./build
cmake -DBUILD_ALL:BOOL=TRUE ..
cmake --build . --config Release
```
- If all went well, it will complete successfully, and you will find all your binaries in the `./build/Release` directory.

#### Windows 10 <a name="build-windows"></a>

##### Prerequisites

- Install [Visual Studio 2017 Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15&page=inlineinstall);
- When installing Visual Studio, it is **required** that you install **Desktop development with C++** and the **VC++ v140 toolchain** when selecting features. The option to install the v140 toolchain can be found by expanding the "Desktop development with C++" node on the right. You will need this for the project to build correctly;
- Make sure that bundled cmake version is 3.10 or higher.

##### Building

- From the start menu, open "x64 Native Tools Command Prompt for vs2017";
- And the run the following commands:
```
git clone --recurse-submodules https://github.com/qwertycoin-org/qwertycoin-gui
cd qwertycoin-gui
md build
cd build
cmake -G "Visual Studio 15 2017 Win64" -DBUILD_ALL:BOOL=TRUE ..
cmake --build . --config Release
```
- If all went well, it will complete successfully, and you will find all your binaries in the `.\build\src\Release` directory;
- Additionally, a `.sln` file will have been created in the `build` directory. If you wish to open the project in Visual Studio with this, you can.

#### Apple macOS <a name="build-apple"></a>

##### Prerequisites

- Install Xcode and Developer Tools;
- Install [cmake](https://cmake.org/). See [here](https://stackoverflow.com/questions/23849962/cmake-installer-for-mac-fails-to-create-usr-bin-symlinks) if you are unable to call `cmake` from the terminal after installing;
- Install git.

##### Building

- After installing dependencies run simple script:
```
git clone --recurse-submodules https://github.com/qwertycoin-org/qwertycoin-gui
cd ./qwertycoin-gui
mkdir ./build
cd ./build
cmake -DBUILD_ALL:BOOL=TRUE ..
cmake --build . --config Release
```
- If all went well, it will complete successfully, and you will find all your binaries in the `./build/Release` directory.

## Donate <a name="donate"></a>

```
QWC: QWC1K6XEhCC1WsZzT9RRVpc1MLXXdHVKt2BUGSrsmkkXAvqh52sVnNc1pYmoF2TEXsAvZnyPaZu8MW3S8EWHNfAh7X2xa63P7Y
```
```
BTC: 1DkocMNiqFkbjhCmG4sg9zYQbi4YuguFWw
```
```
ETH: 0xA660Fb28C06542258bd740973c17F2632dff2517
```
```
BCH: qz975ndvcechzywtz59xpkt2hhdzkzt3vvt8762yk9
```
```
XMR: 47gmN4GMQ17Veur5YEpru7eCQc5A65DaWUThZa9z9bP6jNMYXPKAyjDcAW4RzNYbRChEwnKu1H3qt9FPW9CnpwZgNscKawX
```
```
ETN: etnkJXJFqiH9FCt6Gq2HWHPeY92YFsmvKX7qaysvnV11M796Xmovo2nSu6EUCMnniqRqAhKX9AQp31GbG3M2DiVM3qRDSQ5Vwq
```

### Downloads <a name="downloads"></a>
| Operating System | Download                                 |
| ---------------- | ---------------------------------------- |
| Windows 32       | https://releases.qwertycoin.org/get/gui/latest/win32 |
| Windows 64       | https://releases.qwertycoin.org/get/gui/latest/win64 |
| macOS 10.8 & Later | https://releases.qwertycoin.org/get/gui/latest/macos |
| Linux DEB         | https://releases.qwertycoin.org/get/gui/latest/linux/deb |
| Linux RPM     | https://releases.qwertycoin.org/get/gui/latest/linux/rpm |
| Other platforms      | https://releases.qwertycoin.org |

### Useful Links <a name="usefullinks"></a>
<table>
<tr><td>Website</td><td>https://qwertycoin.org</td></tr>
<tr><td>Bitcointalk ANN</td><td>https://bitcointalk.org/index.php?topic=2881418.0</td></tr>
<tr><td>Explorer</td><td>https://explorer.qwertycoin.org</td></tr>
<tr><td>Pool Explorer</td><td>https://explorer.qwertycoin.org/#network</td></tr>
<tr><td>Node Map</td><td>https://nodes.qwertycoin.org</td></tr>
<tr><td>Wallets</td><td>https://releases.qwertycoin.org</td></tr>
<tr><td>Web Wallet</td><td>https://myqwertycoin.com</td></tr>
<tr><td>Masternode Setup</td><td>https://github.com/qwertycoin-org/qwertycoin/wiki</td></tr>
<tr><td>Blockfolio</td><td>https://blockfolio.com/#get-app</td></tr>
<tr><td>CoinGecko</td><td>https://www.coingecko.com/en/coins/qwertycoin</td></tr>
<tr><td>Delta</td><td>https://delta.app</td></tr>
<tr><td>Discord</td><td>https://qwertycoin.org/discord</td></tr>
<tr><td>Twitter</td><td>https://twitter.com/Qwertycoin_QWC</td></tr>
</table>

#### Thanks <a name="thanks"></a>

Cryptonote Developers, Bytecoin Developers, Monero Developers, Karbo Developers, The Qwertycoin Community

### Copypasta for license when editing files  <a name="copypasta"></a>

Hi Qwertycoin contributor, thanks for forking and sending back Pull Requests. Extensive docs about contributing are in the works or elsewhere. For now this is the bit we need to get into all the files we touch. Please add it to the top of the files, see [lib/Global/CryptoNoteConfig.h](https://github.com/qwertycoin-org/qwertycoin/blob/master/lib/Global/CryptoNoteConfig.h) for an example.

```
// Copyright (c) 2018-2021, The Qwertycoin Developers
//
// Please see the included LICENSE file for more information.
```
