Install Podman without desktop on Windows WSL2
=========

WSL（Windows Subsystem for Linux） is a new feature of Windows 10 that enable you to run Linux distros directly on Windows system. It has several advantages compared to the virtual machine solution. It's fast, easy to use, seamless combined with Windows file system, it's a great tool and worth you to have a try, especially for Windows embedded system developer. 

But many people might meet some unexpected issue when using WSL. Here's the experience guide how I set up my K210 development environment with WSL2 and Podman. Share with you. 

All have been tested under below environment:  
* Windows 11 22621(22H2), running Ubuntu 22.04.1 LTS on WSL2
* Windows 10 19045(22H2), running Ubuntu 22.04.1 LTS on WSL2

## Prerequisites
This article requires you have WSL2 installed on your Windows system, either Win10 or Win11.
Refer to Microsoft documentation to install it: [WSL installation guide](https://learn.microsoft.com/en-us/windows/wsl/install)

## Enable systemd
By default, WSL2 doesn't support systemd, let's enable it manually.
### For Win11
Microsoft has provided an official guide to enable systemd on Win11.   
First you need to edit the WSL config file `/etc/wsl.conf` and add the following content to it:
```bash
[boot]
systemd=true
```
Then we restart WSL2:
```bash
WSL.exe --shutdown
```
After that we run WSL2 distro again, and check if systemd is working correctly:
```bash
systemctl list-unit-files --type=service
```

### For Win10
Win10 doesn't have official support for systemd, but we can still enable it by following the steps as below:
```bash
git clone git@github.com:DamionGans/ubuntu-wsl2-systemd-script.git
cd ubuntu-wsl2-systemd-script
```
Let's make some modifications to the script, open the `ubuntu-wsl2-systemd-script.sh` file with your favorite editor, and change the following line:
```bash
diff --git a/enter-systemd-namespace b/enter-systemd-namespace
index d7a949d..3ab88c5 100755
--- a/enter-systemd-namespace
+++ b/enter-systemd-namespace
@@ -38,11 +38,11 @@ fi
USER_HOME="$(getent passwd | awk -F: '$1=="'"$SUDO_USER"'" {print $6}')"
if [ -n "$SYSTEMD_PID" ] && [ "$SYSTEMD_PID" != "1" ]; then
     if [ -n "$1" ] && [ "$1" != "bash --login" ] && [ "$1" != "/bin/bash --login" ]; then
-        exec /usr/bin/nsenter -t "$SYSTEMD_PID" -a \
+        exec /usr/bin/nsenter -t "$SYSTEMD_PID" -m -p \
             /usr/bin/sudo -H -u "$SUDO_USER" \
             /bin/bash -c 'set -a; [ -f "$HOME/.systemd-env" ] && source "$HOME/.systemd-env"; set +a; exec bash -c '"$(printf "%q" "$@")"
     else
-        exec /usr/bin/nsenter -t "$SYSTEMD_PID" -a \
+        exec /usr/bin/nsenter -t "$SYSTEMD_PID" -m -p \
             /bin/login -p -f "$SUDO_USER" \
             $([ -f "$USER_HOME/.systemd-env" ] && /bin/cat "$USER_HOME/.systemd-env" | xargs printf ' %q')
     fi
 ```
Then run the script:
```bash
sudo ./ubuntu-wsl2-systemd-script.sh
```
Once finished, reboot your WSL2 distro firstly:
```bash
WSL.exe --shutdown
```
Then run WSL2 distro again, check if systemd is working correctly:
```bash
systemctl list-unit-files --type=service
```


## Install podman
Many articles tell you to install podman-desktop on Windows, but it's not the way I use, I want to install podman without desktop. Instead of it I install podman on WSL2 directly. Here's how to do it:
```bash
. /etc/os-release
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/xUbuntu_${VERSION_ID}/ /" | sudo tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
sudo apt-get install -y ca-certificates
curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/xUbuntu_${VERSION_ID}/Release.key -o Release.key
sudo apt-key add ./Release.key
sudo apt update
sudo apt install -y podman
```
After that, let's check the podman installation status
```bash
$ podman info
host:
  arch: amd64
  buildahVersion: 1.23.1
  cgroupControllers: []
  cgroupManager: cgroupfs
  cgroupVersion: v1
  conmon:
...
```

## Install podman-compose
In K210 docker, we use docker-compose yaml file to config the build server. It requires podman-compose installed in your system to run build server:
```commandline
$ wget https://raw.githubusercontent.com/containers/podman-compose/devel/podman_compose.py
$ sudo cp podman_compose.py /usr/local/bin/podman-compose
$ sudo chmod +x /usr/local/bin/podman-compose
$ sudo apt install python3-pip
$ pip install python-dotenv
```
Use below command to check the podman-compose installation status
```bash
$ podman-compose --version
```
