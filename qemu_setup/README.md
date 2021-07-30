# QEMU setup to run WRTD with the SPEC150T and FMC ADC

## 1. QEMU installation and image creation

To install QEMU with `apt`, run the following command:
```bash
sudo apt install qemu-system-x86_64
```

To create the disk image, you need to install `qemu-img`, available through `apt`:
```bash
sudo apt install qemu-utils
```
Then you can run the following command to generate an 10Go image file for your virtual system's disk:
```bash
qemu-img create -f qcow2 disk.img 10G
```

You now need to get an iso file to install your preferred Linux distribution, which I will refer as `linux.iso`.
To install your distribution on the disk image, run the following command:
```bash
qemu-system-x86_64 -enable-kvm -cpu host -m 2G -boot d -cdrom linux.iso -hda disk.img
```
- `-m 2G` will allocate 2Go of RAM for QEMU
- `-enable-kvm -cpu host` will use virtualization capabilities of the processor to speed up the emulation

After installation is complete, you can launch your virtual system with the following command:
```bash
qemu-system-x86_64 -enable-kvm -cpu host -m 2G -hda disk.img
```
