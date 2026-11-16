## File-based Covert Channel

File-lock covert storage channel in C. `sender` encodes bits by holding (1) or releasing (0) a lock on `/tmp/cc.lock`; `attacker` probes it with `flock(LOCK_EX | LOCK_NB)`. Both sync on a shared `CLOCK_MONOTONIC` frame grid and sample mid-slot. Settings live in `common.h`.

```sh
make && ./sender    # terminal 1
./attacker          # terminal 2
```
