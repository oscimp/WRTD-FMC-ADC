# Experiments to synchronize the Si570 clock of the ADC to White Rabbit

When experimenting with data acquisition with the FMC ADC 100M and WRTD, we realized that acquisition timing were not as tight as we expected.
Here is a description of the experiment that was conduced:

We had 2 SPEC boards installed inside two different computers, both connected to the same White Rabbit network.
The two ADCs were capturing a signal from a broadband noise generator, with the same cable length on both sides.
This way we would know that our 2 two acquisitions are synchronized if the data matched between the two.
We would acquire a few samples by triggering the two ADCs at a common date using WRTD functionnalities.

The expected results should look like the following:

<img src="acquisition.png" width="500">

However, we would often get a time offset between the two measures as shown here:

<img src="acquisition_delay.png" width="500">

This offset would range from -10ns to +10ns (a sampling period since we acquire at 100MHz).
We repeated the experiment to look at the repartition of the delay in probability terms:

<img src="correlation_histogram.png" width="500">

It turns out the repartition seems continuous, which makes it difficult to counter.
Ideally we would like no delay (this means about 60ps for us, which is what White Rabbit promises).
But if we had a discrete repartition, we could at least find a way to handle cases in software (see the end of this document for why we would need to do that).

After discussing on CERN's forums (https://forums.ohwr.org/t/is-the-fmc-adc-100m-14b-4cha-clock-synchronized-with-white-rabbit-on-the-spec150/848703), it became certain that the cause of these delays is the fact that the ADC clock (Si570) is not disciplined to White Rabbit.
To be clear, there is a clock disciplined to White Rabbit on the SPEC for WRTD, but the ADC does not use it.
Researchers from CERN did not care about this precision loss for their measurements, but it is much more critical in our case.

This documents synthetizes our researches towards the goal of discipling the ADC clock to White Rabbit.

## 1. Accessing the Si570 registers from userspace

This section will cover a way to access registers (read and write) using sysfs (the Linux virtual file system at /sys).
This may or may not be useful towards the goal of disciplining the clock since we will necessarely need to modify the FPGA design.
It at least proves that we can change the frequency of the ADC clock using the I2C interface that is accessible through the FMC connector.

To access the registers, we will be modifying the fmc-adc-100m14bcha driver.
The patches provided in the patch directory should be applied in order using the following commands:
```bash
cd $BUILD_DIR/fmc-adc-100m14b4cha-sw
patch -p 1 < <path to this repository>/si570/patch/<patch name>.patch
```
After doing your modifications to the source files, you will need to recompile the driver.
Comment out the 2 lines that checkout a branch and patch files in the script `fmc_adc_100m_build.sh` located in the ohwr-build-scripts repository used to intall WRTD.
Place your terminal at the scripts directory of the ohwr-build-script repository and run the following commands.
```bash
rm $BUILD_DIR/built.fmc-adc-100m14b4cha-sw
sh fmc_adc_100m_build.sh
```
Now reboot, flash the FPGA, and you should get access to some of the Si570 registers if you look inside `/sys/bus/zio/devices/adc-100m-XXXX/cset0`.

Now for some explanation of what the patch does to achieve this access:

### I2C communication

To communicate with the Si570 clock via I2C, the FPGA design uses a component from OpenCores called "I2C master" available here: https://opencores.org/projects/i2c .
To be more exact, it rather uses a wrapper over this component, and it must also be noted that the documentation found on OpenCores for the component has errors in it (notably the register adresses that are wrong).

#### Accessing the I2C master

This component is wired on a Wishbone bus that also allows access to the ADC's registers and various other interfaces available through the FMC connector.
If we take a look at the `fmc-adc-100m14b4cha.h` header, we can see it defines offsets for some of the interfaces available, but not for the I2C one.
We can just add a macro `ADC_I2C_OFF` with the correct offset that can be found by investigating the HDL code (`0x1600`).

However, the kernel uses its own virtual address space to access these hardware parts.
Looking at the driver probe function in `fa-core.c`, we can see a base address is attributed in the `fa_top_level` member of the device structure.
And this structure also has other members to store an address for each wishbone interface, which add an offset to the base address.
We can then add a member called `fa_i2c_base` in the definition of `struct fa_dev` inside `fmc-adc-100m14b4cha-sw.h`, and set it equal to `fa->fa_top_level + ADC_I2C_OFF` in the probe function.

#### Using the I2C master

Now we can communicate with the I2C master component using `fa_readl`/`fa_writel` and the address stored in the `fa_i2c_base`.
We just need to figure out the register addresses of this component, and the series of instructions to use to read or write from the clock's registers.

There are 5 registers of interest for the I2C master component, which are labelled `CTR`, `TXR`, `RXR`, `CR`, and `TR`.
We can add these in the `zfadc_dregs_enum` enumeration defined in `fmc-adc-100m14b4cha.h`.
These are all 8bit registers, which we can describe in the `zfa_regs` array defined inside `fa-regtable.c`.
Since the addresses provided by the official OpenCores documentation are wrong, I had to manually find them in the HDL code.
One oddity is also that register addresses need to bit-shifted twice to the left because of granularity issues caused by the wrapper used.

Some of these registers are bitfields so I defined a few macros inside `fa-i2c.h` to help masking individual bits.

Finally, reading and writing to the Si570 clock registers is done thanks to two functions that perform the right sequence of register accesses to the I2C master.
These are named `fa_i2c_read` and `fa_i2c_write` and are written inside an additional `fa-i2c.h` header file.
Read the comments of these functions if you want to undersand the sequence of instructions that is done.

### Interacting with sysfs

For every register we want to access, we can add an entry in the `zfad_cset_ext_zattr` array defined inside `fa-zio-drv.c`.
For each entry, we need to specify a string that will appear as the filename in sysfs, the read/write permissions, and a unique ID that should be defined in the `zfadc_dregs_enum` enumeration mentionned earlier.

Then we can modify the two functions `zfad_conf_set` (for writing) and `zfad_info_get` (for reading) to tell the kernel what to do when a pseudo file from sysfs is read or written to.
In our case we want to add special cases for I2C registers.
We don't want the default case to happen, and instead call `fa_i2c_read` or `fa_i2c_write`.
As described in the Si570 documentation, the clock registers can contain several information, so it is necessary to reconstruct the wanted value by masking and bit shifting.
Also it is required to read registers before writing to them if we want to preserve bits that don not concern the variable we want to change.

In the case of RFREQ (see the Si570 documentation), the 38bit value does not fit in the 32bit unsigned integer used by the driver to read/write from sysfs.
To compensate for that fact, I provided two pseudo files to read the integer part and the fractionnal part separately (see the next paragraph for more details).

Here is how to interact with sysfs from your shell:
```bash
# To read just use cat
cat /sys/bus/zio/devices/adc-100m14b-XXXX/cset0/si570-hsdiv

# To write use echo and output redirection
echo 1 > /sys/bus/zio/devices/adc-100m14b-XXXX/cset0/si570-hsdiv
```

### RFREQ conversion

The Si570 documentation describes the RFREQ value as a 38bit fixed point number.
The 10 most significant bits are the integer part and the 28 remaining are the fractionnal part.
This is not a usual format to work with so I wrote two conversion functions, transforming the 38bit number (stored inside a 64bit unsigned integer as the lower bits) to a double precision floating point number from the IEEE norm, and the reverse operation.

These function can be accessed with the `rfreq.h` header and are named respectively `rfreq_to_float` and `float_to_rfreq`.

If you have two 32bit unsigned integer containing the integer and fractionnal part of RFREQ, you can reconstruct RFREQ into a 64bit unsigned integer using this formula:
```c
uint64_t rfreq = (((uint64_t) rfreq_int) << 28) | rfreq_frac;
```

## 2. Recompiling the bitstream

If modifications are to be made to the FPGA design, it will be necessary to resynthetize the bitstream.
To do so a few tools are required.

### Dependencies from package managers

With `apt` for Debian
```
sudo apt install flex bison
```

### hdl-make

Clone the repository and run the installation script.
```bash
git clone https://ohwr.org/project/hdl-make.git
cd hdl-make
python3 setup.py install --user
```

*Notice that the current (Aug. 2022) version of hdlmake will generate a Makefile that is not 
functional due to a missing space between the command and the argument. To correct, plase replace line 157 of ``hdlmake/tools/makefilesyn.py`` from ``\t\t$(TCL_INTERPRETER)$@.tcl`` to ``\t\t$(TCL_INTERPRETER) $@.tcl`` before setting up.*

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

*Notice that a license is needed to synthetsize for the LX150T FPGA, possibly a XUP (Xilinx University Program) license for academic users, but the free WebPack will not allow to complete the synthesis.*

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

After completion, you should see a file named `wrtd_ref_spec150t_adc.bin`. Just copy it to `/lib/firmware` and next time you program the FPGA it will use it.

## 3. What is left to explore

To synchronize the ADC clock on the White Rabbit clock, we will need to modify the FPGA design in some way.
The issue is that it seems extremely difficult from our perspective to simulate the whole design or even some parts of it, so debugging any modifications will be a pain.

To explore the FPGA design, look at the code inside `wrtd/hdl/top/wrtd_ref_spec150t_adc` and inside the various dependencies inside `wrtd/dependencies`.
Here is a general idea of what some dependencies define:
- `general-cores` mainly provides components for wishbone connections (adapters, crossbars...)
- `wr-cores` is where components performing clock synchronization are defined
- `fmc-adc-100m14b4cha-gw` contains the wiring of the all the FMC pins, and the necessary components to extract the ADC data
- `spec` contains the base design that instantiate the core components to synchronize the SPEC clock to White Rabbit

### Phase lock loop inside the FPGA

In theory the White Rabbit core (https://ohwr.org/projects/wr-cores) provides us with a PLL component that we could use to regulate the Si570 frequency.
It is left to understand how to use the IP and insert it into the existing design.
Looking at what has already been done to synchronize the SPEC clock could help.
Some documentation can be found here: https://ohwr.org/project/wr-cores/wikis/Wrpc-release-v42.

A component is also provided to interface between the soft-PLL component of the WR-core, and the I2C master that communicates with the Si570.
However it does not seem to be used in any projects from CERN, and does not seem up to date with the rest of the cores.
Dimitrios Lampridis from CERN mentionned we could ask him by email if we have questions about this component.
Here is the component in question: https://ohwr.org/project/wr-cores/blob/proposed_master/modules/wr_si57x_interface/wr_si57x_interface.vhd

Also since the Si570 is on the ADC and due to the wiring, we don't have direct access to the ADC clock signal from the FPGA.
We will need to get a reconstructed clock signal that can be found as an output of the data deserializer, which will necessarily have a phase difference from the original signal.
In short, the data sent by the ADC uses a clock signal that is 4 times the Si570 frequency, and the deserializer receives that clock to process data, which it scales to its needs.

### Phase lock loop in userspace

If we want to use the I2C access described in part 1 for a PLL, we would need a way to get a measure of the frequency difference between the Si570 and White Rabbit clocks.
This means adding a component that calculates that measurement, and providing a way to access the result through the main Wishbone bus which could finally be read from userspace.

#### Starting point

In the SPEC driver, we can spot a macro defining an offset for White Rabbit Core (WRC) registers in `spec_core_fpga.h`, but which isn't used anywhere.
This header is generated using cheby and the source file `spec_base_regs.cheby`.
This should in theory allow us to access the existing soft-PLL registers in a similar way to what we did for the I2C.
Some of the addresses defined in the header are used (thermometer ID for example) and can be used as references.

However the cheby description defines a `0x1000` sized memory space for WRC registers, and when looking at the FPGA design we see two Wishbone crossbars leading from the SPEC register block to the soft-PLL.
These crossbar notably define an offset of `0x0002_0000` from the first to the second, which supposedly overflows the allocated memory space.
For now I am unsure what addresses to use within the driver in order to read anything from the soft-PLL registers.

<img src="register_access.png">

### Clock domains

The Si570 is supposed to run at 100MHz, and the White Rabbit synchronized clock used in the PLL runs at 125MHz.
This means the PLL will divide both frequencies, and we will then have 5 different edges on which the synchronization can happen.
Knowing which edge is the right one is not a trivial task, but if we get there we will have at least transformed a uniform random distribution of the sample delay to a discrete one which is more manageable.

During a meeting with CERN engineers, some potential changes to the PTP core were mentioned to account for clock domain crossing issues.
However this is not yet implemented so we need to find our own way for now.

## 4. Merging DIO DPLL and WRTD

As of Aug. 2022, Tomasz Wlostowski provided a functional digital-PLL control of the slave Si570 from the White-Rabbit-controlled master Si570 connected to the SPEC150T FPGA. The comparison of the two synthesis procedures are as follows. Let us emphasize that the DIO is using the Lattice LM32 softcore CPU whereas the WRTD is additionally using the RISC-V, hence the need to install two different cross-compilation toolchains, both using heavily outdated versions of gcc (the RISC-V toolchain **must** be https://ohwr.org/project/wrpc-sw/wikis/Documents/Project-Attachments and not any newer version of gcc which would result in a code that freezes):

DIO DPLL | WRTD |
---------|------|
Top level HDL:
https://ohwr.org/project/wr-cores/tree/tom-wrpc-v5-oversampled-ddmtd/top/spec_ref_design | |
Synthesis directory:
https://ohwr.org/project/wr-cores/tree/tom-wrpc-v5-oversampled-ddmtd/syn/spec_ref_design | |
Recursive clone wr-cores | |
$ git clone https://ohwr.org/project/wr-cores.git | |
$ git submodule update --init --recursive | |
$ git checkout tom-wrpc-v5-oversampled-ddmtd | |
Open the .xise project in the syn/spec_ref_design subdirectory and build the bitstream. | |
For the WRPC software: | |
$ git clone https://ohwr.org/project/wrpc-sw | |
$ git submodule update --init --recursive | |
$ git checkout tom-wrpc-v5-oversampled-dmtd | |
$ make menuconfig | |
In the target board menu, select "SPEC Board with a silabs aux oscillator (TEST)”. Then save the config and do | |
$ make | |
Two binaries to be loaded to the board are generated. | |
Get the spec-sw driver at https://ohwr.org/project/spec-sw | |
as root: spec-fwloader spec_wr_ref_top.bin | |
as root: spec-cl wrc.bin | |
as root: spec-vuart # open the virtual UART console... | |

To be continued...
