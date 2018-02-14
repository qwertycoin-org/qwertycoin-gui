**1. Clone wallet sources**

```
git clone https://github.com/qwertycoin-org/qwertycoin.git
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
