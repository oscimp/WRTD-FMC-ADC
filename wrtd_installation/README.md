# Installing WRTD for the SPEC150T and FMC ADC

We will be using the scripts provided by Dimitrios Lampridis to install all dependencies:
https://gitlab.cern.ch/dlamprid/ohwr-build-scripts

## 1. Installing dependencies 

With `apt` for Debian:
```bash
sudo apt install linux-headers-amd64 build-essential gcc git patch sudo curl lua5.1 python-setuptools python-yaml python-decorator libreadline-dev
```
You may need to install `python-is-python3` so Python works properly when running the scripts later.

With `yum` for CentOS:
You will need CentOS Plus
```bash
sudo yum --enablerepo centosplus install kernel-plus kernel-plus-headers kernel-plus-devel
```
Reboot using CentOS Plus.
```bash
sudo yum install gcc git patch curl python-setuptools python-decorator python-yaml readline-devel
```
