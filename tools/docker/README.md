Build CanMV with Podman or Docker
=========


## Prerequisites
To build CanMV with Docker or Podman, you need to install either Docker or Podman on your computer. It supports Windows, Linux and macOS. Installation on those platforms is not covered in this document. Please refer to the official documentation of Docker or Podman for installation instructions.  

Require one of the following tool combinations:
* [Podman] & [podman-compose]
* [Docker] & [docker-compose]

## About the docker and podman
Docker and podman are both containerization tools. They are similar in many ways, but there are also some differences. For example, docker is a commercial product, and podman is an open source project. Docker is more popular than podman, but podman is more lightweight than docker. You can choose one of them according to your needs.
I would suggest you to use podman, because it is more lightweight and open source.  
The following instructions are based on podman rootless mode with Win11 WSL2, but it should work on other operating systems.

For Linux user, usually it's easy to follow the official documentation to install docker or podman. But for Windows user, it's not that easy. Here's the experience guide how I set up my K210 development environment with WSL2 and Podman. Share with you: [Install Podman without desktop on Windows WSL2](podman_on_wsl2.md).

## Start build server
I used to start the build server in the background, then attach to it with an interactive console. Here's how to do it:  
Firstly, let's start the build server:
```bash
cd canmv
cd tools/docker
./start.sh
```
You can check the build server status to make sure it's running well:

```bash
podman ps -a
```
If the build server is running, you will see the following output:
```bash
CONTAINER ID  IMAGE                               COMMAND     CREATED         STATUS             PORTS       NAMES
95398252bc60  ghcr.io/kendryte/k210_build:latest  bash        14 minutes ago  Up 14 minutes ago              k210_build
```
## Build firmware
Then let's get into the k210_build server shell then start to build:
```bash
podman exec -it k210_build bash
```
By default, the start script will mount the CanMV root directory to `/workspace` of the build server, `/workspace` is the default working directory of k210_build server where you start. Check the docker-compose.yml for details, you can also do your own customization there.    
Get into your target project folder and run the following commands to build the firmware. Here I use canmv_k210 as an example:
```bash
cd projects/canmv_k210
python project.py clean
python project.py build
```
If everything goes well, you will see the following output:  
```bash
...
[ 99%] Built target main
[ 99%] Building C object CMakeFiles/canmv.dir/exe_src.c.o
[100%] Linking CXX executable canmv
-- Generating .bin firmware at /workspace/projects/canmv_k210/build/canmv.bin
============= firmware =============
   text    data     bss     dec     hex filename
2032581   34499  855464 2922544  2c9830 /workspace/projects/canmv_k210/build/canmv.elf
[100%] Built target canmv
==================================
time: Mon Nov  7 11:57:06 2022
build end, time last:23.68s
==================================
```

Done!   
You can find the firmware in the build folder of your project, and burn it to your board with proper tools.  

## Reference
* [WSL installation guide](https://learn.microsoft.com/en-us/windows/wsl/install)
* [Docker installation](https://docs.docker.com/engine/install/ubuntu/)
* [Docker-compose installation](https://docs.docker.com/compose/install/)
* [Podman installation](https://podman.io/getting-started/installation)
* [Podman-compose installation](https://github.com/containers/podman-compose)