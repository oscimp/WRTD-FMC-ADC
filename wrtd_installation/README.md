# Installing WRTD for the SPEC150T and FMC ADC

We will be using the scripts provided by Dimitrios Lampridis to install all dependencies:
https://gitlab.cern.ch/dlamprid/ohwr-build-scripts

## 1. Installing dependencies 

With `apt` for Debian:
```bash
sudo apt install linux-headers-amd64 build-essential gcc git patch sudo curl lua5.1 python-setuptools python-yaml python-decorator libreadline-dev
```
You may need to install `python-is-python3` so Python works properly when running the scripts later.

---

With `yum` for CentOS:

You will need CentOS Plus
```bash
sudo yum --enablerepo centosplus install kernel-plus kernel-plus-headers kernel-plus-devel
```
Reboot using CentOS Plus.
```bash
sudo yum install gcc git patch curl python-setuptools python-decorator python-yaml readline-devel
```

## 2. Running the scripts

Start by cloning the scripts:
```bash
git clone https://gitlab.cern.ch/dlamprid/ohwr-build-scripts.git
```

You will then need to patch the repository:
```bash
cd ohwr-build-scripts
patch -p 1 < ../ohwr-build-scripts-patch.patch
```

You should set the variable `BUILD_DIR` to a directory where all sources and built files will end up.
Finally, you should just need to run the installation script:
```bash
cd scripts
sh wrtd_ref_spec150t_adc_install.sh
```
