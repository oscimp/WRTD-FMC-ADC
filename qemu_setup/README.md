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
Then you can run the following command to generate a 10Go image file for your virtual system's disk:
```bash
qemu-img create -f qcow2 disk.img 10G
```

You now need to get an iso file to install your preferred Linux distribution, which I will refer as `linux.iso`.
To install your distribution on the disk image, run the following command and follow the installer's intructions:
```bash
qemu-system-x86_64 -enable-kvm -cpu host -m 2G -boot d -cdrom linux.iso -hda disk.img
```
- `-m 2G` will allocate 2Go of RAM for QEMU
- `-enable-kvm -cpu host` will use virtualization capabilities of the processor to speed up the emulation

After installation is complete, you can launch your virtual system with the following command:
```bash
qemu-system-x86_64 -enable-kvm -cpu host -m 2G -hda disk.img
```

## 2. Setting up network for QEMU

Running `sudo ./setup.sh` will configure the network on the host side. You can look at the first half of the script to see that it creates a virtual interface named `tap0` and routes the host interface properly to get network access.

_Note: You should provide the script with the `HOST_INTERFACE` variable containing the name of your host interface, otherwise it will default to `eth0`._

You then need to add the following options when launching QEMU:
```
-device virtio-net,netdev=network0 -netdev tap,id=network0,ifname=tap0,script=no,downscript=no,vhost=on 
```

Now **inside QEMU**, you will need to set up the interfaces.
If you installed Debian, you can add the following lines in `/etc/network/interfaces`
```
iface <interface name> inet static
	address 192.168.0.2
	netmask 255.255.255.0
	gateway 192.168.0.1
```
If you installed CentOS, you can write the following lines in a file at `/etc/sysconfig/network-scripts/ifcfg-<interface name>`:
```
DEVICE=<interface name>
ONBOOT=yes
IPADDR=192.168.0.2
NETMASK=255.255.255.0
GATEWAY=192.168.0.1
```

You may need to edit `/etc/resolv.conf` to setup the DNS servers, and also provide your proxy with the `http_proxy` and `https_proxy` variables.
Hopefully now you have access to your network from QEMU.

## 3. Accessing the SPEC board inside QEMU
The second half of the `setup.sh` script will activate the VFIO kernel modules, unbind the driver on the host side, and assign the board.

You should now be able to launch QEMU with the following options added, where `<PCI ID>` is what the setup script should have printed in the form `XX:XX.X` (the colons need to be escaped with a baskslash in the command argument if written manually):
```
-device intel-iommu,caching-mode=on -device vfio-pci,host=<PCI ID>
```

If QEMU does not launch, you should try adding the following boot option to your host computer (inside `/sys/grub/grub.cfg` most likely):
```
vfio_iommu_type1 allow_unsafe_interrupts=1
```

At last, you should see the SPEC board if you run `lspci | grep CERN` inside QEMU.

## 4. Setting up a shared directory
Having a shared directory between the host computer and QEMU can facilitate a bunch of task.

If you are using CentOS in QEMU, you will need to install CentOS Plus and reboot:
```
yum --enablerepo centosplus install kernel-plus
```

On the host side, you only need to create a directory to use, and add the following options when launching QEMU:
```
-virtfs local,path=<host shared directory>,mount_tag=host0,security_model=passthrough,id=host0
```

**Inside QEMU**, you can also create a directory, such as `/mnt/shared`, and then mount with the following command:
```
mount -t 9p -o trans=virtio host0 /mnt/shared
```

## 5. Summary and launcher script
To ease the task, a setup and launcher scripts have been created.

You must execute the setup script (with sudo permissions) once after each reboot of the host computer.
You should provide it with the `HOST_INTERFACE` variable containing the name of the interface to use for QEMU.

You can then use `sudo ./laucher.sh disk.img` to launch QEMU with all the options detailled earlier.
You can provide a `SHARED_DIR` variable to specify your host shared directory, otherwise it will default to `/mnt/shared`.
