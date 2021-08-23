# PCI class code changer

## Description of the issue

You may encounter an error on some machines when the SPEC driver is loaded (it is the case for the Raspberry Pi CM4 for example).
If you see a similar log in `dmesg`, read the following solution:
```
spec-fmc-carrier 0000:01:00.0 can't enable device: BAR 0 [mem 0x00000000-0x000fffff 64bit] not claimed
spec-fmc-carrier 0000:01:00.0 Failed to enable PCI device (-22)
spec-fmc-carrier probe of 0000:01:00.0 failed with error -22
```
This issue comes from the BIOS which in not handling properly the PCI class code `0x00`.
The solution proposed here consists in reprogramming the EEPROM containing the code and changing it to `0xFF`.

If you want to explore other solutions, see this discussion:
https://forums.ohwr.org/t/spec-pci-class-code/848718

## How to use this tool

The script `install.sh` downloads sources from CERN's gitlab, applies the patch, initializes the submodules, and builds the necessary tools.

The script `change_pci_class.sh` reads the EEPROM of the Gennum chip used by the SPEC150T to communicate via PCIe.
It then modifies the PCI class code to be `0xFF` and rewrites the content to the EEPROM.

Notes:
- The script was written assuming that there is a single PCI card from CERN listed by `lspci`. It should be straightforward to modify it so it can handle several cards if necessary.
- If the class code needs to be reprogrammed to something else than `0xFF` it should also be easy to modify the code.
- In case you are using QEMU, you need to execute the script from QEMU, it will not work on the host computer when passing the SPEC board to QEMU.
