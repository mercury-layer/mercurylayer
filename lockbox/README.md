## Lockbox server

To run:

1. Install vcpkg package manager, following the instruction [here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash).
2. Install `ninja` build system (`sudo apt-get -y install ninja-build`).
3. Then run the commands below:

```bash
$ mkdir -p build
$ cd build
$ cmake --preset=vcpkg ..
$ cmake --build .
```

4. Then, to run the server: `./MercuryLockbox`