#!/bin/bash

UTARGET=$(uname)

if [ "$UTARGET" = "Darwin" ]; then
  export UPLATFORM="mac"
elif [ "$UTARGET" = "Linux" ]; then
  export UPLATFORM="linux_$(uname -m)"
else
  # Fallback to something...
  export UPLATFORM="$UTARGET"
fi

if [ "$UPLATFORM" = "mac" ]; then
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
fi

echo "Attempting to build UEFITool NE for ${UPLATFORM}..."

UEFITOOL_VER=$(cat uefitool.cpp                        | grep ^version             | cut -d'"' -f2)
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
  # -flto is flawed on CI atm
  if [ "$UPLATFORM" = "mac" ]; then
    qmake $3 QMAKE_CXXFLAGS+=-flto QMAKE_LFLAGS+=-flto CONFIG+=optimize_size || exit 1
  else
    qmake $3 CONFIG+=optimize_size || exit 1
  fi

  make || exit 1

  # Archive
  if [ "$1" = "UEFITool" ]; then
    if [ "$UPLATFORM" = "mac" ]; then
      strip -x UEFITool.app/Contents/MacOS/UEFITool || exit 1
      zip -qry dist/"${1}_${2}_${UPLATFORM}.zip" UEFITool.app "${4}" || exit 1
    else
      strip -x "$1" || exit 1
      zip -qry dist/"${1}_${2}_${UPLATFORM}.zip" "${1}" "${4}" || exit 1
    fi
  else
    strip -x "$1" || exit 1
    zip -qry ../dist/"${1}_${2}_${UPLATFORM}.zip" "${1}" "${4}" || exit 1
  fi

  # Return to parent
  if [ "$1" != "UEFITool" ]; then
    cd - || exit 1
  fi
}

rm -rf dist
mkdir -p dist || exit 1

build_tool UEFITool    "$UEFITOOL_VER"     uefitool.pro
build_tool UEFIPatch   "$UEFIPATCH_VER"    uefipatch.pro patches.txt
build_tool UEFIReplace "$UEFIREPLACE_VER"  uefireplace.pro

exit 0
