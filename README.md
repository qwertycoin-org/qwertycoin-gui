![image](https://cdn.qwertycoin.org/images/press/other/qwc-github.png)

[![Build Status](https://travis-ci.org/qwertycoin-org/qwertycoin.svg?branch=stage_1)](https://travis-ci.org/qwertycoin-org/qwertycoin)
[![Build status](https://ci.appveyor.com/api/projects/status/l3o455xl2l9lhrlu/branch/master?svg=true)](https://ci.appveyor.com/project/qwertycoin-org/qwertycoin-gui/branch/master)

### How To Compile

##### Prerequisites

- You will need the following packages: boost (1.64 or higher), QT Library (5.9.0 orhigher) cmake (3.10 or higher), git, gcc (4.9 or higher), g++ (4.9 or higher), make, and python. Most of these should already be installed on your system.
- For example on ubuntu: `sudo apt-get install build-essential python-dev gcc g++ git cmake libboost-all-dev qtbase5-dev`
- After this you can compile your Qwertycoin

**1. Clone wallet sources**

```
git clone --recurse-submodules https://github.com/qwertycoin-org/qwertycoin
```

**2. Set symbolic link to coin sources at the same level as `src`. For example:**

```
ln -s ../qwertycoin cryptonote
```

Alternative way is to create git submodule:

```
git submodule add https://github.com/qwertycoin-org/qwertycoin.git cryptonote
```

**3. Build**

```
mkdir build && cd build && cmake .. && make
```

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
