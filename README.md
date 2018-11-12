# UEFITool

UEFITool is a viewer and editor of firmware images conforming to UEFI Platform Interface (PI) Specifications.

![UEFITool icon](https://raw.githubusercontent.com/LongSoft/UEFITool/new_engine/UEFITool/icons/uefitool_64x64.png "UEFITool icon")  
[![Build Status](https://travis-ci.org/LongSoft/UEFITool.svg?branch=master)](https://travis-ci.org/LongSoft/UEFITool) [![Scan Status](https://scan.coverity.com/projects/17209/badge.svg?flat=1)](https://scan.coverity.com/projects/17209)


## Very Brief Introduction to UEFI

Unified Extensible Firmware Interface or UEFI is a post-BIOS firmware specification originally written by Intel for Itanium architecture and than adapted for X86 systems.  
The first EFI-compatible x86 firmwares were used on Apple Macintosh systems in 2006 and PC motherboard vendors started putting UEFI-compatible firmwares on their boards in 2011.  
In 2015 there are numerous systems using UEFI-compatible firmware including PCs, Macs, Tablets and Smartphones on x86, x86-64 and ARM architectures.  
More information on UEFI is available on [UEFI Forum official site](http://www.uefi.org/faq) and in [Wikipedia](http://en.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface).  
  
## Very Brief Introduction to UEFITool

UEFITool is a cross-platform open source application written in C++/Qt, that parses UEFI-compatible firmware image into a tree structure, verifies image's integrity and provides a GUI to manipulate image's elements.  
Project development started in the middle of 2013 because of the lack of cross-platform open source utilities for tinkering with UEFI images.  

In the beginning of 2015 the major refactoring round was started to make the program compatible with newer UEFI features including FFSv3 volumes and fixed image elements. 
It's in development right now with the following features still missing:
* Editor part, i.e image reconstruction routines
* Console UI

The missing parts are in development and the version with a new engine will be made as soon as image reconstruction works again.

## Derived projects

There are some other projects that use UEFITool's engine:
* UEFIExtract, which uses ffsParser to parse supplied firmware image into a tree structure and dumps the parsed structure recursively on the FS. Jethro Beekman's [tree](https://github.com/jethrogb/uefireverse) utility can be used to work with the extracted tree.
* UEFIFind, which uses ffsParser to find image elements containing a specified pattern. It was developed for [UBU](http://www.win-raid.com/t154f16-Tool-Guide-News-quot-UEFI-BIOS-Updater-quot-UBU.html) project.
* [OZMTool](https://github.com/tuxuser/UEFITool/tree/OZM/OZMTool), which uses UEFITool's engine to perform various "hackintosh"-related firmware modifications.

## Alternatives

Right now there are some alternatives to UEFITool that you could find useful too:
* **[Fiano](https://github.com/linuxboot/fiano)** by Google and Facebook. Go-based cross-platform open source toolset for modifying UEFI firmware images.
* **[PhoenixTool](http://forums.mydigitallife.info/threads/13194-Tool-to-Insert-Replace-SLIC-in-Phoenix-Insyde-Dell-EFI-BIOSes)** by [AndyP](http://forums.mydigitallife.info/members/39295-andyp). Windows-only freeware GUI application written in C#. Used mostly for SLIC-related modifications, but it not limited to this task. Requires Microsoft .NET 3.5 to work properly. Supports unpacking firmware images from various vendor-specific formats like encrypted HP update files and Dell installers.  
* **[uefi-firmware-parser](https://github.com/theopolis/uefi-firmware-parser)** by [Teddy Reed](https://github.com/theopolis). Cross-platform open source console application written in Python. Very tinker-friendly due to use of Python. Can be used in scripts to automate firmware patching.
* **[Chipsec](https://github.com/chipsec/chipsec)** by Intel. Cross-platform partially open source console application written in Python and C. Can be used to test Intel-based platforms for various security-related misconfigurations, but also has NVRAM parser and other components aimed to firmware modification.
* **MMTool** by AMI. Windows-only proprietary application available to AMI clients. Works only with Aptio4- and AptioV-based firmware images, but has some interesting features including OptionROM replacement and microcode update. Must be licensed from AMI.
* **H2OEZE** by Insyde. Windows-only proprietary application available to Insyde clients. Works only with InsydeH2O-based firmware images. Must be licensed from Insyde.
* **SCT BIOS Editor** by Phoenix. Windows-only proprietary application available to Phoenix clients. Works only with Phoenix SCT-based firmware images. Must be licensed from Phoenix.

## Installation

You can either use [pre-built binaries for Windows and macOS](https://github.com/LongSoft/UEFITool/releases) or build a binary yourself.  
* To build a binary that uses Qt library (UEFITool) you need a C++ compiler and an instance of [Qt5](https://www.qt.io) library. Install both of them, get the sources, generate makefiles using qmake (`qmake UEFITool.pro`) and use your system's make command on that generated files (i.e. `nmake release`, `make release` and so on).
* To build a binary that doesn't use Qt (UEFIExtract, UEFIFind), you need a C++ compiler and [CMAKE](https://cmake.org) utility to generate a makefile for your OS and build environment. Install both of them, get the sources, generate makefiles using cmake (`cmake UEFIExtract`) and use your system's make command on that generated files (i.e. `nmake release`, `make release` and so on).

## Known issues

* Some vendor-specific firmware update files can be opened incorrectly or can't be opened at all. This includes encrypted HP update files, Dell HDR and EXE files, some InsydeFlash FD files and so on. Enabling support for such files will require massive amount of reverse-engineering which is almost pointless because the updated image can be obtained from BIOS chip where it's already decrypted and unpacked.
* Intel Firmware Interface Table (FIT) editing is not supported right now. FIT contains pointers to various image components that must be loaded before executing the first CPU instruction from the BIOS chip. Those components include CPU microcode updates, binaries and settings used by BIOS Guard and Boot Guard technologies and some other stuff. More information on FIT can be obtained [here](http://downloadmirror.intel.com/18931/eng/Intel%20TXT%20LAB%20Handout.pdf).
* Builder code is still not ready.
