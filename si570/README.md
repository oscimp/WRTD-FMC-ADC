# Experiments to synchronize the Si570 clock of the ADC to White Rabbit

## Accessing the Si570 registers from userspace

This section will cover a way to access registers (read and write) using sysfs (the Linux virtual file system at /sys).
This may or may not be useful towards the goal of disciplining the clock since we will necessarely need to modify the FPGA design.
It at least proves that we can change the frequency of the ADC clock using the I2C interface that is accessible trough the FMC connector.

TODO

### RFREQ conversion

The Si570 documentation describes the RFREQ value as a 38 bits fixed point number.
The 10 most significant bits are the decimal part and the 28 remaining are the fractionnal part.
This is not a usual format to work with so I wrote two conversion functions, transforming the 38 bits number (stored inside a 64bit unsigned integer as the lower bits) to a double precision floating point number from the IEEE norm, and the reverse operation.

These function can be accessed with the `rfreq.h` header and are named respectively `rfreq_to_float` and `float_to_rfreq`.

## Recompiling the bitstream

If modifications are to be made to the FPGA design, it will be necessary to resynthetize the bitstream.
To do so a few tools are required.

### Dependencies from package managers

With `apt` for Debian
```
sudo apt install flex bison
```

### hdl-make

Simply clone the repository and run the installation script.
```bash
git clone https://ohwr.org/project/hdl-make.git
cd hdl-make
python3 setup.py install --user
```

### wishbone-gen

You will need to specify the path to the tool `wbgen2`.

If you have installed WRTD software from the scripts, it should be located in your build directory.
```bash
export WBGEN2=$BUILD_DIR/wishbone-gen/wbgen2
```

### RISC-V toolchain

A special toolchain from CERN is necessary to compile for RISC-V 32bit (the FPGA runs a RISC-V CPU).
Thankfully, binaries are provided:
- For Debian: https://ohwr-packages.web.cern.ch/ohwr-packages/riscv_toolchains/riscv-debian_testing.tar.xz
- For CentOS 7: https://ohwr-packages.web.cern.ch/ohwr-packages/riscv_toolchains/riscv-centos7.tar.xz

After extracting the binaries, you need to tell the cross compiler to use these tools:
```bash
export CROSS_COMPILE_TARGET=<install path>/riscv_toolchain/bin/riscv32-elf-
```

### Xilinx ISE

An account is needed to download the official Linux installer:
https://www.xilinx.com/member/forms/download/xef.html?filename=Xilinx_ISE_DS_Lin_14.7_1015_1.tar

Extract the files and launch `xsetup`. If the installer keeps giving an error when specifying the installation directory, try using `bin/lin64/xsetup` instead.

You need to specify your license file, and finally you can add ISE to you PATH by sourcing the following file (only for your current shell).
```bash
source <install path>/Xilinx/14.7/ISE_DS/settings64.sh
```

### WRTD repository

Clone the WRTD repository, go to version 1.0.0 and initilize the submodules.
```
git clone https://ohwr.org/project/wrtd.git
cd wrtd
git checkout v1.0.0
git submodule update --init
```

### Synthesis

You must configure a git username for the WRTD repository (if not already done) before starting the synthesis. Use `git config user.name <your username>`.

At last, you should be able to generate the bitstream:
```bash
cd hdl/syn/wrtd_ref_spec150t_adc
hdlmake
make
```
This will take roughly 15 minutes.

