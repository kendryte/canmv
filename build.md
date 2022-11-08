Build CanMV from source code
=========

Only support Linux(recommend) and macOS


## Get source code

* Clone by https link

```
git clone https://github.com/kendryte/canmv.git
```

* Then get submodules

```
git submodule update --recursive --init
```

It will regiter and clone all submodules, if you don't want to register all submodules, cause some modules is unnecessary, just execute
```
git submodule update --init
# or 
git submodule update --init path_to_submodule
# or
git submodule update --init --recursive path_to_submodule
```

## Install dependencies

Ubuntu for example:

```
sudo apt update
sudo apt install python3 python3-pip build-essential cmake
sudo pip3 install -r requirements.txt

```

If macOS:
* Install cmake by brew or download dmg file from [cmake official website](https://cmake.org/download/)
* Ensure command `cmake --version` can be executed in terminal, in macOS maybe you need to add cmake bin dir to `PATH` in `~/.bashrc` or `~/.zshrc` like:
```
export PATH=$PATH:/Applications/CMake.app/Contents/bin/
```


> recommend `python3` instead of python2

Check `CMake` version by 

```
cmake --version
```

The `cmake` version should be at least `v3.9`, if not, please install latest `cmake` manually from [cmake website](https://cmake.org/download/)




## Download toolchain

Download the latest toolchain from [here](https://github.com/kendryte/kendryte-gnu-toolchain/releases) (macOS and linux), or [kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz](https://github.com/kendryte/kendryte-gnu-toolchain/releases/download/v8.2.0-20190409/kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz) (for linux)

And extract to `/opt/kendryte-toolchain/`

For example:

```
wget https://github.com/kendryte/kendryte-gnu-toolchain/releases/download/v8.2.0-20190409/kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz
sudo tar -Jxvf kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz -C /opt
ls /opt/kendryte-toolchain/bin
```

## Configure project

* Switch path to `hello_world` project directory

```
cd projects/hello_world
```

or canmv project

```
cd projects/canmv_k210
```

* Configure toolchain path

The default toolchain path is `/opt/kendryte-toolchain/bin`,
and default toolchain pfrefix is `riscv64-unknown-elf-`.

If you have copied toolchain to `/opt`, just use default, and it is highly recommend!!

Or you can customsize your toolchain path by 

```
python3 project.py --toolchain /opt/kendryte-toolchain/bin --toolchain-prefix riscv64-unknown-elf- config 
```
> Clean config to default by command `python3 project.py clean_conf`

* Configure project

Usually, just use the default configuration.

If you want to customsize project modules, execute command:

```
python3 project.py menuconfig
```

This command will display a configure panel with GUI,
then change settings and save configuration.

For more details, please refer to the [projects/README.md](projects/README.md)

## Build

```
python3 project.py build
```

And clean project by:

```
python3 project.py clean
```

Clean all build files by:

```
python3 project.py distclean
```

The make system is generated from `cmake`, 
you must run

```
python3 project.py rebuild
```

to rebuild make system after you add/delete source files or edit kconfig files




## Flash (Burn) to board


For example, you have one `CanMV k210` board:

```
python3 project.py -B auto -p /dev/ttyACM0 -b 1500000 flash
```

For `Maixduino` board:

```
python3 project.py -B maixduino -p /dev/ttyUSB0 -b 1500000 -S flash
```

`-B` means board, `-p` means board serial device port, `-b` means baudrate, `-S` or `--Slow` means download at low speed but more stable mode.

the configuration saved in `.flash_conf.json` except args: `--sram`(`-s`)、`--terminal`(`-t`)、`--Slow`(`-S`)
You don't need to confiure again the next time you burn firmware, just use:
```
python3 project.py flash
```
or 
```
python3 project.py -S flash
```


More parameters help by :

```
python3 project.py --help
```

## Build with Docker or Podman
We provide Dockerfile and scripts to build the project in a container. This is useful if you don't want to install all the dependencies on your host system.
Here's a guide how to build the project with Docker: [Docker Guide](tools/docker/README.md)
## Others

* Code conventions: TODO
* Build system: refer to [c_cpp_project_framework](https://github.com/Neutree/c_cpp_project_framework)







