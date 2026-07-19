CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

PROJECT(UEFITool_everything)

ADD_SUBDIRECTORY(UEFIExtract)
ADD_SUBDIRECTORY(UEFIFind)
ADD_SUBDIRECTORY(UEFIEdit)

# UEFITool GUI requires Qt6 for cmake-based builds (upstream constraint).
# Qt5 users should build the GUI with qmake instead: cd UEFITool && qmake-qt5 uefitool.pro && make
# To allow building the console utilities alone on systems without Qt6 (e.g. Fedora with Qt5 only),
# the GUI subdir is added only when Qt6 is actually available.
FIND_PACKAGE(Qt6 QUIET COMPONENTS Widgets)
IF(Qt6_FOUND)
    ADD_SUBDIRECTORY(UEFITool)
ELSE()
    MESSAGE(STATUS "Qt6 not found: skipping UEFITool GUI (build it with qmake instead).")
ENDIF()
