# Password-strength dictionary provenance

`dict-src.h` is the deterministic embedded dictionary input for the vendored
`zxcvbn-c` source in this directory. It was generated from
`tsyrogit/zxcvbn-c` tag `v2.0`, commit
`4d46debda8ea4484ac452b86c6a06ca6b830e102`, using its six tracked word lists:

```sh
make -f makefile dict-src.h
```

Expected SHA-256 value:

```text
57a7b0d08ea5cbd359ffb7680df38ebb5bf662111c04696adfd68560126e5a0b  dict-src.h
```

Embedding the generated source avoids a mutable runtime data-file dependency
and keeps dictionary matching read-only and safe for concurrent UI bindings.
The upstream source and generated data retain the Tony Evans copyright and
license terms embedded in `zxcvbn.c` and `zxcvbn.h`. The word lists originate
from the original Dropbox zxcvbn project as documented by upstream.
