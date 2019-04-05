![image](https://cdn.qwertycoin.org/images/press/other/qwc-github-3.png)
#### Master Build Status
[![Build Status](https://travis-ci.org/qwertycoin-org/qwertycoin-gui.svg?branch=master)](https://travis-ci.org/qwertycoin-org/qwertycoin-gui) [![Build status](https://ci.appveyor.com/api/projects/status/1g1cxw31kqep753n/branch/master?svg=true)](https://ci.appveyor.com/project/Qwertycoin/qwertycoin-gui/branch/master) [![codecov](https://codecov.io/gh/qwertycoin-org/qwertycoin-gui/branch/master/graph/badge.svg)](https://codecov.io/gh/qwertycoin-org/qwertycoin-gui)

#### Development Build Status
[![Build Status](https://travis-ci.org/qwertycoin-org/qwertycoin-gui.svg?branch=dev)](https://travis-ci.org/qwertycoin-org/qwertycoin-gui) [![Build status](https://ci.appveyor.com/api/projects/status/1g1cxw31kqep753n/branch/dev?svg=true)](https://ci.appveyor.com/project/Qwertycoin/qwertycoin-gui/branch/dev) [![codecov](https://codecov.io/gh/qwertycoin-org/qwertycoin-gui/branch/dev/graph/badge.svg)](https://codecov.io/gh/qwertycoin-org/qwertycoin-gui)

### Installing

We offer binary images of the latest releases here: https://releases.qwertycoin.org

If you would like to compile yourself, read on.

### How To Compile

#### Linux

##### Prerequisites

- You will need the following packages: boost (1.64 or higher), QT Library (5.9.0 or higher) cmake (3.10 or higher), git, gcc (4.9 or higher), g++ (4.9 or higher), make, and python. Most of these should already be installed on your system.
- For example on ubuntu: `sudo apt-get install build-essential python-dev gcc g++ git cmake libboost-all-dev qtbase5-dev`
- After this you can build your Qwertycoin Zero Wallet.

##### Building

- After installing dependencies run simple script:
```
git clone --recurse-submodules https://github.com/qwertycoin-org/qwertycoin-gui
cd ./qwertycoin-gui
mkdir ./build
cd ./build
cmake ..
cmake --build . --config Release
```
- If all went well, it will complete successfully, and you will find all your binaries in the `./build/Release` directory.

#### Windows 10

##### Prerequisites

- Install [Visual Studio 2017 Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15&page=inlineinstall);
- When installing Visual Studio, it is **required** that you install **Desktop development with C++** and the **VC++ v140 toolchain** when selecting features. The option to install the v140 toolchain can be found by expanding the "Desktop development with C++" node on the right. You will need this for the project to build correctly;
- Make sure that bundled cmake version is 3.10 or higher.

##### Building

- From the start menu, open "x64 Native Tools Command Prompt for vs2017";
- And the run the following commands:
```
git clone https://github.com/qwertycoin-org/qwertycoin-gui
cd qwertycoin-gui
md build
cd build
cmake -G "Visual Studio 15 2017 Win64" ..
cmake --build . --config Release
```
- If all went well, it will complete successfully, and you will find all your binaries in the `.\build\Release` directory;
- Additionally, a `.sln` file will have been created in the `build` directory. If you wish to open the project in Visual Studio with this, you can.

#### Apple

##### Prerequisites

- Install Xcode and Developer Tools;
- Install [cmake](https://cmake.org/). See [here](https://stackoverflow.com/questions/23849962/cmake-installer-for-mac-fails-to-create-usr-bin-symlinks) if you are unable to call `cmake` from the terminal after installing;
- Install git.

##### Building

- After installing dependencies run simple script:
```
git clone https://github.com/qwertycoin-org/qwertycoin-gui
cd ./qwertycoin-gui
mkdir ./build
cd ./build
cmake ..
cmake --build . --config Release
```
- If all went well, it will complete successfully, and you will find all your binaries in the `./build/Release` directory.


## Donate

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

#### Thanks

Cryptonote Developers, Bytecoin Developers, Monero Developers, Karbo Developers, Qwertycoin Community
