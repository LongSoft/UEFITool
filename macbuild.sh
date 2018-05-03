#!/bin/bash

if [ ! -d /opt/qt56sm ]; then
  curl -L -o /tmp/qt-5.6.3-static-mac.zip https://github.com/distdb/qtbuilds/blob/master/qt-5.6.3-static-mac.zip?raw=true || exit 1
  qtsum=$(shasum -a 256 /tmp/qt-5.6.3-static-mac.zip | cut -f1 -d' ')
  qtexpsum="214d22d8572ea6162753c8dd251d79275f3b22d49204718c637d722409e0cfcb"
  if [ "$qtsum" != "$qtexpsum" ]; then
    echo "Qt hash $qtsum does not match $qtexpsum"
    exit 1
  fi
  sudo mkdir -p /opt || exit 1
  cd /opt || exit 1
  sudo unzip -q /tmp/qt-5.6.3-static-mac.zip || exit 1
  cd - || exit 1
fi

export PATH="/opt/qt56sm/bin:$PATH"

echo "Attempting to build UEFITool for macOS..."

UEFITOOL_VER=$(cat uefitool.cpp                        | grep ^version             | cut -d'"' -f2)
UEFIEXTRACT_VER=$(cat UEFIExtract/uefiextract_main.cpp | grep '"UEFIExtract [0-9]' | cut -d'"' -f2 | cut -d' ' -f2)
UEFIFIND_VER=$(cat UEFIFind/uefifind_main.cpp          | grep '"UEFIFind [0-9]'    | cut -d'"' -f2 | cut -d' ' -f2)
UEFIPATCH_VER=$(cat UEFIPatch/uefipatch_main.cpp       | grep '"UEFIPatch [0-9]'   | cut -d'"' -f2 | cut -d' ' -f2)
UEFIREPLACE_VER=$(cat UEFIReplace/uefireplace_main.cpp | grep '"UEFIReplace [0-9]' | cut -d'"' -f2 | cut -d' ' -f2)

build_tool() {
  echo "Building $1 $2"
  # Check version
  if [ "$(echo "$2" | grep '^[0-9]*\.[0-9]*\.[0-9]*$')" != "$2" ]; then
    echo "Invalid $1 version!"
    exit 1
  fi
  # Tools are in subdirectories
  if [ "$1" != "UEFITool" ]; then
    cd "$1" || exit 1
  fi

  # Build
  qmake $3 QMAKE_CXXFLAGS+=-flto QMAKE_LFLAGS+=-flto CONFIG+=optimize_size || exit 1
  make || exit 1

  # Archive
  if [ "$1" = "UEFITool" ]; then
    strip -x UEFITool.app/Contents/MacOS/UEFITool || exit 1
    zip -qry dist/"UEFITool_${UEFITOOL_VER}_mac.zip" UEFITool.app "${4}" || exit 1
  else
    strip -x "$1" || exit 1
    zip -qry ../dist/"${1}_${2}_mac.zip" "${1}" "${4}" || exit 1
  fi

  # Return to parent
  if [ "$1" != "UEFITool" ]; then
    cd - || exit 1
  fi
}

rm -rf dist
mkdir -p dist || exit 1

build_tool UEFITool    "$UEFITOOL_VER"     uefitool.pro
build_tool UEFIExtract "$UEFIEXTRACT_VER"  uefiextract.pro
build_tool UEFIFind    "$UEFIFIND_VER"     uefifind.pro
build_tool UEFIPatch   "$UEFIPATCH_VER"    uefipatch.pro patches.txt
build_tool UEFIReplace "$UEFIREPLACE_VER"  uefireplace.pro

exit 0
