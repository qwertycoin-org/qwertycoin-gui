![image](https://qwertycoin.org/files/qwc-github.jpg)

[![Build status](https://ci.appveyor.com/api/projects/status/l3o455xl2l9lhrlu/branch/master?svg=true)](https://ci.appveyor.com/project/qwertycoin-org/qwertycoin-gui/branch/master)

### How To Compile

##### Prerequisites

- You will need the following packages: boost (1.55 or higher), QT Library (5.9.0 orhigher) cmake, git, gcc (4.9 or higher), g++ (4.9 or higher), make, and python. Most of these should already be installed on your system.
- For example on ubuntu: `sudo apt-get install build-essential python-dev gcc g++ git cmake libboost-all-dev `
- After this you can compile your Qwertycoin

**1. Clone wallet sources**

```
git clone https://github.com/qwertycoin-org/qwertycoin-gui
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
