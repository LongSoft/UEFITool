UEFITool
========
.. image:: https://raw.githubusercontent.com/LongSoft/UEFITool/master/uefitool.ico
.. image:: https://scan.coverity.com/projects/1812/badge.svg?flat=1
    :target: https://scan.coverity.com/projects/1812/

|
| UEFITool is a cross-platform C++/Qt program for parsing, extracting and modifying UEFI firmware images.
| It supports parsing of full BIOS images starting with the flash descriptor or any binary files containing UEFI volumes.
| Original development was started `here <http://forums.mydigitallife.info/threads/48979-UEFITool-UEFI-firmware-image-viewer-and-editor>`_ at MDL forums as a cross-platform analog to `PhoenixTool <http://forums.mydigitallife.info/threads/13194-Tool-to-Insert-Replace-SLIC-in-Phoenix-Insyde-Dell-EFI-BIOSes>`_'s structure mode with some additional features, but the program's engine was proven to be usefull for another projects like `UEFIPatch <http://www.insanelymac.com/forum/topic/285444-uefipatch-uefi-patching-utility/>`_, `UBU <http://www.win-raid.com/t154f16-Tool-quot-UEFI-BIOS-Updater-quot-UBU.html>`_ and `OZMTool <http://www.insanelymac.com/forum/topic/299711-ozmtool-an-ozmosis-toolbox/>`_.

Installation
------------

| You can either use `pre-built binaries for Windows and OSX <https://github.com/LongSoft/UEFITool/releases/latest>`_ or build a binary yourself. 
| To build a binary you need a C++ compiler and an instance of Qt4/Qt5 library for it. 
| Install both of them, get the sources, generate makefiles using qmake (*qmake UEFITool.pro*) and use your make command on that generated files (i.e. *nmake release*, *make release* and so on).

Usage
-----

| The program can be started directly without any arguments or supplied with a single argument - a path to the UEFI image file to open after start.
|
| The program window is divided into three panels: **Structure**, **Information** and **Messages**.
| Structure of the image is represented as a tree of elements with different names, types and subtypes. If you select an element, **Information** panel will show the available information about the selected element based on it's type and contents.
| **Messages** panel show all messages from the engine, including structure warnings and search results. Most of messages can be double-clicked to select the element that causes the message.
|
| You can open a menu on each tree element to see what operations are possible for the selected element. This can include various types of **Extract**, **Insert** and **Replace** operations, as well as **Remove** and **Rebuild**.
| **Extract** has two variants: **Extract as is** and **Extract body**. The difference is that **Extract as is** extracts the element with it's header (GUID, size, attributes and other structure-related information are located there), and **Extract body** extracts the element data only. 
| **Replace** has the same two variants as **Extract** with the same meaning.
| **Insert** has the three different variants: **Insert before**, **Insert after** and **Insert into**, which is only available for UEFI volumes and encapsulation sections.
| **Remove** marks an element for removal on image reconstuction.
| **Rebuild** marks an element for rebuilding on image reconstruction. Normally, all elements that aren't marked for rebuild won't be changed at all and if you need to correct some structure error (i.e. invalid data checksum) you must mark an element for rebuild manually. If you change an element all it's parents up to the tree root will be marked for rebuild automatically. If UEFI volume is marked for rebuild all uncompressed PEI files in it will also be marked for rebuild because they must be rebased in the reconstructed image to maintain the executable-in-place constraint.
| 
| There is also a search function available from the *File* menu, you can search all tree elements for a specified hexadecimal pattern (spaces are not counted, dot symbol (.) is used as placeholder for a single hex digit), a specified GUID (rules are the same as for hex except for spaces) and a specified text (either Unicode or ASCII, case sensitive or not). Search results will be added into **Messages** panel, if anything is found.
|
| After you've finished the modifications, you need to initiate image reconstruction using *Save image file* command from the *File* menu. If anything goes wrong on the reconstruction, an error will pop up, otherwise the program will prompt if you need to open the reconstructed file. Don't rush it, because reconstruction process can also generate some usefull messages, which will be lost if you open the reconstructed file immediatelly.

Known issues
------------
* Some images has non-standard calculation of base address of TE images, so the program can rebase them incorrectly after modifications. Will be solved ASAP.
* Some images may not work after modification because of no FIT table support implemented yet. It's on my high priority features list, so I hope it will be corrected soon.
* The program is meant to work with BIOS images, not some vendor-specific BIOS update files, that is why some of that update file either can\t be opened at all or return errors on reconstruction. If someone wants to write an unpacker for such crappy files - I will be glad to use it.
* AMI-specific features like NCBs, ROM_AREA structure and other things like that can't be implemented by me because of the NDA I have.
