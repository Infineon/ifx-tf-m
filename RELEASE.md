# Infineon Customized Trusted Firmware-M Release Notes

## What's Included?
Trusted Firmware-M (TF-M) implements the Secure Processing Environment (SPE)
for Arm Cortex-M based platforms. This aligns the reference implementation of the platform security architecture
with the PSA Certified guidelines. Thus, TF-M allows relevant chips and devices to become PSA Certified.

## Changelog

### v2.3.300
* Add support for PSC3P6/PSC3M6 devices.
* Restructured TF-M libraries for better modularity:
    * **ifx-tf-m** - Unified library containing TF-M source code (src/ directory), common for all platforms
    * **ifx-tf-m-port** - Platform-specific configurations, linker scripts, and build files
    * This replaces the previous platform-specific libraries (ifx-tf-m-pse84epc2, ifx-tf-m-pse84epc4, etc.)
* Updated TF-M to released TF-M version 2.3.0

### v2.1.600
* Updated version per dependent assets update.
* Removed automatic Python virtual environment creation and Python module installation from TF-M
  build scripts; users must install required Python modules in their build Python environment.

### v2.1.500
* Updated version to support memory reporting in ModusToolbox (MTB) 3.7.

### v2.1.400
* Updated version to include latest releases of dependent assets: ifx-mbedtls, mtb-dsl-pse8xxgp and other platform-specific libraries.

### v2.1.300
* Removed support of IFX_PDL_SECURE_SERVICES
* Added support of IFX_MTB_SRF
* Renamed the ifx-trusted-firmware-m-ns library to ifx-tf-m-ns
* Instead of the single ifx-trusted-firmware-m library, dedicated libraries have been created for each platform:
    * ifx-tf-m-pse84epc2 for PSE84 EPC2
    * ifx-tf-m-pse84epc4 for PSE84 EPC4
    * ifx-tf-m-psc3 for PSC3

### v2.1.200
* Added support of PSE84 EPC4
* Added support of IFX_PDL_SECURE_SERVICES
* Added support of the Edge Protect Configurator
* Added support of memory layout configuration via Memory configurator integrated into the Device Configurator
* Deprecated the ifx-src-trusted-firmware-m library. TF-M sources are part of
  the ifx-trusted-firmware-m library.
* Added a new ifx-trusted-firmware-m-ns library to integrate builds of the TF-M non-secure interface into
  a non-secure project.

### v2.1.100
* TF-M 2.1.0
* Added support of PSC3

### v2.0.10
* TF-M 2.0.0 with external memory support for PSE84 EPC2

### v1.8.0
* Initial release for TF-M with support for PSE84 EPC2

## Supported Software and Tools
This version of TF-M was validated for compatibility with the following Software and Tools:

| Software and Tools                                                            | Version |
| :---                                                                          | :----:  |
| CMake                                                                         | 3.27.7  |
| Python modules from `tools/requirements.txt`                                  |         |
## More information
Use the following links for more information, as needed:
* [Cypress Semiconductor Corporation (an Infineon company)](https://www.infineon.com)
* [Cypress Semiconductor Corporation (an Infineon company) GitHub](https://github.com/Infineon)
* [Trusted Firmware website](https://www.trustedfirmware.org)
* [TF-M project](https://www.trustedfirmware.org/projects/tf-m)
* [PSA API](https://arm-software.github.io/psa-api)
* [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software)

---
© 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
Technologies AG. All rights reserved.
This software, associated documentation and materials ("Software") is
owned by Infineon Technologies AG or one of its affiliates ("Infineon")
and is protected by and subject to worldwide patent protection, worldwide
copyright laws, and international treaty provisions. Therefore, you may use
this Software only as provided in the license agreement accompanying the
software package from which you obtained this Software. If no license
agreement applies, then any use, reproduction, modification, translation, or
compilation of this Software is prohibited without the express written
permission of Infineon.

Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
Infineon reserves the right to make changes to the Software without notice.
You are responsible for properly designing, programming, and testing the
functionality and safety of your intended application of the Software, as
well as complying with any legal requirements related to its use. Infineon
does not guarantee that the Software will be free from intrusion, data theft
or loss, or other breaches ("Security Breaches"), and Infineon shall have
no liability arising out of any Security Breaches. Unless otherwise
explicitly approved by Infineon, the Software may not be used in any
application where a failure of the Product or any consequences of the use
thereof can reasonably be expected to result in personal injury.
