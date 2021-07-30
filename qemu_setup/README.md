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

## 2. Setting up network for QEMU

Running `sudo ./setup.sh` will configure the network on the host side. You can look at the first half of the script to see that it creates a virtual interface named `tap0` and routes the host interface properly to get network access.
__Note: You should provide the script with the `HOST_INTERFACE` variable containing the name of your host interface, otherwise it will default to `eth0`.__

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
