# UEFITool

UEFITool is a viewer and editor of firmware images conforming to UEFI Platform Interface (PI) Specifications.

![UEFITool icon](https://raw.githubusercontent.com/LongSoft/UEFITool/new_engine/UEFITool/icons/uefitool_64x64.png "UEFITool icon")  
![CI Status](https://github.com/LongSoft/UEFITool/actions/workflows/main.yml/badge.svg?branch=new_engine) [![Scan Status](https://scan.coverity.com/projects/17209/badge.svg?flat=1)](https://scan.coverity.com/projects/17209) [![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=LongSoft_UEFITool&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=LongSoft_UEFITool)


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
* UEFIFind, which uses ffsParser to find image elements containing a specified pattern. It was developed for [UBU](https://winraid.level1techs.com/t/tool-guide-news-uefi-bios-updater-ubu/30357) project.

## Alternatives

Right now there are some alternatives to UEFITool that you could find useful too:
* **[FMMT](https://github.com/tianocore/edk2/tree/master/BaseTools/Source/Python/FMMT)** by TianoCore. Python-based open source toolset for modifying EDK2-based UEFI firmware images. Does not support any IBV customizations, but is _official_, and lives in EDK2 repository.
* **[Fiano](https://github.com/linuxboot/fiano)** by Google and Facebook. Go-based cross-platform open source toolset for modifying UEFI firmware images.
* **[PhoenixTool](https://forums.mydigitallife.net/threads/tool-to-insert-replace-slic-in-phoenix-insyde-dell-efi-bioses.13194)** by [AndyP](https://forums.mydigitallife.net/members/andyp.39295). Windows-only freeware GUI application written in C#. Used mostly for SLIC-related modifications, but it not limited to this task. Requires Microsoft .NET 3.5 to work properly. Supports unpacking firmware images from various vendor-specific formats like encrypted HP update files and Dell installers.
* **[uefi-firmware-parser](https://github.com/theopolis/uefi-firmware-parser)** by [Teddy Reed](https://github.com/theopolis). Cross-platform open source console application written in Python. Very tinker-friendly due to use of Python. Can be used in scripts to automate firmware patching.
* **[Chipsec](https://github.com/chipsec/chipsec)** by Intel. Cross-platform partially open source console application written in Python and C. Can be used to test Intel-based platforms for various security-related misconfigurations, but also has NVRAM parser and other components aimed to firmware modification.

## Installation

You can either use [pre-built binaries for Windows and macOS](https://github.com/LongSoft/UEFITool/releases) or build a binary yourself.  
* To build a binary that uses Qt library (UEFITool) you need a C++ compiler and an instance of [Qt5 or Qt6](https://www.qt.io) library. Install both of them, get the sources, generate makefiles using qmake (`qmake ./UEFITool/uefitool.pro`) and use your system's make command on that generated files (i.e. `nmake release`, `make release` and so on). Qt6-based builds can also use CMAKE as an altearnative build system.
* To build a binary that doesn't use Qt (UEFIExtract, UEFIFind), you need a C++ compiler and [CMAKE](https://cmake.org) utility to generate a makefile for your OS and build environment. Install both of them, get the sources, generate makefiles using cmake (`cmake UEFIExtract`) and use your system's make command on that generated files (i.e. `nmake release`, `make release` and so on). Non-Qt builds can also use Meson as an alternative build system.

## Known issues
* Image editing is currently only possible using an outdated and unsupported UEFITool 0.28 (`old_engine` branch) and the tools based on it (`UEFIReplace`, `UEFIPatch`). This is the top priority [issue #67](https://github.com/LongSoft/UEFITool/issues/67), which is being worked on, albeit slowly (due to the amount of coding and testing required to implement it correctly). 
* Some vendor-specific firmware update files can be opened incorrectly or can't be opened at all. This includes encrypted HP update files, Dell HDR and EXE files, some InsydeFlash FD files and so on. Enabling support for such files will require massive amount of reverse-engineering which is almost pointless because the updated image can be obtained from BIOS chip where it's already decrypted and unpacked.
* Intel Firmware Interface Table (FIT) editing is not supported right now. FIT contains pointers to various image components that must be loaded before executing the first CPU instruction from the BIOS chip. Those components include CPU microcode updates, binaries and settings used by BIOS Guard and Boot Guard technologies and some other stuff. More information on FIT can be obtained [here](https://edc.intel.com/content/www/us/en/design/products-and-solutions/software-and-services/firmware-and-bios/firmware-interface-table/firmware-interface-table/).
* Windows builds of `UEFIExtract` might encouter an issue with folder paths being longer than 260 bytes (`MAX_PATH`) on some input files (see [issue #363](https://github.com/LongSoft/UEFITool/issues/363)). This is a [known Windows limitation](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry), that can be fixed by enabling long paths support via Windows Registry and adding a manifest to the executable file that requires such support. `UEFIExtract` has the required manifest additions since version `A67`, and the required registry file is provided by Microsoft on the page linked above, but this workaround is only awailable starting with Windows 10 build 1067.   

## Bug repellents
* [Coverity Scan](https://scan.coverity.com/projects/17209) - static analyzer for C, C++, C#, JavaScript, Ruby, or Python code.
* [SonarCloud](https://sonarcloud.io/project/overview?id=LongSoft_UEFITool) - cloud-based code analysis service.
* [PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=github&utm_medium=organic&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.
* [CodeQL](https://codeql.github.com/docs/codeql-overview/about-codeql) - code analysis engine developed by GitHub to automate security checks.

