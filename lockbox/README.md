## Lockbox server

To run:

1. Install vcpkg package manager, following the instruction [here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash).
2. Install `ninja` build system (`sudo apt-get -y install ninja-build`).
3. Then run the commands below:

```bash
$ mkdir -p build && cd build
$ cmake --preset=vcpkg ..
$ cmake --build .
```
4. Set the desired key manager in `Settings.toml`. Currently, there are 3 available: `google_kms`, `hashicorp`, `filesystem`.

5. Then, to run the server: `./MercuryLockbox` or `./MercuryLockbox`.