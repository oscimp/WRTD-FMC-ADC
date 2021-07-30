# Experiments to synchronize the Si570 clock of the ADC to White Rabbit

## Recompiling the bitstream

If modification are to be made to the FPGA design, you will need to resynthetize the bitstream.
To do so you will need a few tools.

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

If you have installed the WRTD software from the scripts, then it should be located in you build directory.
```bash
export WBGEN2=$BUILD_DIR/wishbone-gen/wbgen2
```

### RISC-V toolchain

A special toolchain from CERN is necessary to compile for RISC-V 32bit (the FPGA runs a RISC-V CPU).
Thankfully, binaries are provided:
- For Debian: https://ohwr-packages.web.cern.ch/ohwr-packages/riscv\_toolchains/riscv-debian\_testing.tar.xz
- For CentOS 7: https://ohwr-packages.web.cern.ch/ohwr-packages/riscv\_toolchains/riscv-centos7.tar.xz

After extracting the binaries, you then need to tell the cross compiler to use these tools:
```bash
export CROSS_COMPILE_TARGET=<install path>/riscv_toolchain/bin/riscv32-elf-
```
