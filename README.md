# WRTD-FMC-ADC
White Rabbit Trigger Distribution (WRTD) for the reference design SPEC150T + FMC ADC 100MHz 14 bits 4 channels

The documentation includes, and should be read in the following order:

- `qemu_setup` describes how to configure QEMU to be used with this project (completely optionnal)
- `wrtd_installation` covers how to install all the dependencies needed for this application
- `wrtd_usage` explains how to use the project and provides some examples
- `si570` synthetizes the researches that were made to synchronize the ADC clock to White Rabbit. This document concludes with work in progress and perspectives on locking the ADC clock on the White Rabbit referenced oscillators, which is currently (Aug. 2021) not the case.

## List of abbreviations:

- ADC: **A**nalog to **D**igital **C**onverter, system used to acquire data from sensors and process it digitally. We are using this ADC card for this project: https://ohwr.org/project/fmc-adc-100m14b4cha/wikis/home.

- CM4: **C**ompute **M**odule **4**, a board model from Raspberry Pi that can be used for this project.

- FMC: **F**PGA **M**ezzanine **C**ard, a standard used to communicate between an FPGA and an extension card. The ADC used for this project is plugged in the FMC slot of the SPEC board.

- PCI: **P**eripheral **C**omponent **I**nterconnect, a computer bus commonly used to plug extension boards (graphics card, sound card, or the SPEC in our case) to a computer. PCI express (often written PCIe) is an evolution of PCI.

- SPEC: **S**imple **P**CI **E**xpress FMC **C**arrier, name of the board that we use as our WRTD node. The SPEC150T model is the same as the SPEC but with a bigger FPGA chip: https://ohwr.org/project/spec150/wikis/home.

- WRTD: **W**hite **R**abbit **T**rigger **D**istribution, a project from CERN to distribute triggers between hosts connected to a White Rabbit network.
