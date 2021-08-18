# PCI class code changer

The script `install.sh` downloads sources from CERN's gitlab, applies the patch, initializes the submodules, and builds the necessary tools.

The script `change_pci_class.sh` reads the EEPROM of the Gennum chip used by the SPEC150T to communicate via PCIe.
It then modifies the PCI class code to be `0xFF` and rewrites the content to the EEPROM.

Notes:
- The script was written assuming that there is a single PCI card from CERN listed by `lspci`. It should be straightforward to modify it so it can handle several cards if necessary.
- If the class code needs to be reprogrammed to something else than `0xFF` it should also be easy to modify the code.
- In case you are using QEMU, you need to execute the script from QEMU, it will not work on the host computer when passing the SPEC board to QEMU.
