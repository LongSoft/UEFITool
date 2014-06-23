#!/usr/bin/env bash

# Capstone Disassembler Engine
# By Nguyen Anh Quynh <aquynh@gmail.com>, 2013>

# Note: to cross-compile "nix32" on Linux, package gcc-multilib is required.


# build iOS lib for all iDevices, or only specific device
function build_iOS {
	${MAKE} clean
	SDK=`xcrun --sdk iphoneos --show-sdk-path`
	GCC_BIN=`xcrun --sdk iphoneos -f gcc`
	GCC_BASE="$GCC_BIN -Os -Wimplicit -isysroot $SDK"
	if (( $# == 0 )); then
		# build for all iDevices
		GCC="$GCC_BASE -arch armv7 -arch armv7s -arch arm64"
	else
		GCC="$GCC_BASE -arch $1"
	fi
	${MAKE} CC="$GCC"
}

function build {
	${MAKE} clean

	if [ ${CC}x != x ]; then
		${MAKE} CC=$CC
	else
		${MAKE}
	fi
}

function install {
	# Mac OSX needs to find the right directory for pkgconfig
	if [ "$(uname)" == "Darwin" ]; then
		# find the directory automatically, so we can support both Macport & Brew
		PKGCFGDIR="$(pkg-config --variable pc_path pkg-config | cut -d ':' -f 1)"
		if [ ${PKGCFGDIR}x != x ]; then
			if [ ${CC}x != x ]; then
				${MAKE} CC=$CC PKGCFGDIR=$PKGCFGDIR install
			else
				${MAKE} PKGCFGDIR=$PKGCFGDIR install
			fi
		else
			if [ ${CC}x != x ]; then
				${MAKE} CC=$CC install
			else
				${MAKE} install
			fi
		fi
	else	# not OSX
		if test -d /usr/lib64; then
			if [ ${CC}x != x ]; then
				${MAKE} LIBDIRARCH=lib64 CC=$CC install
			else
				${MAKE} LIBDIRARCH=lib64 install
			fi
		else
			if [ ${CC}x != x ]; then
				${MAKE} CC=$CC install
			else
				${MAKE} install
			fi
		fi
	fi
}

function uninstall {
	# Mac OSX needs to find the right directory for pkgconfig
	if [ "$(uname)" == "Darwin" ]; then
		# find the directory automatically, so we can support both Macport & Brew
		PKGCFGDIR="$(pkg-config --variable pc_path pkg-config | cut -d ':' -f 1)"
		if [ ${PKGCFGDIR}x != x ]; then
			${MAKE} PKGCFGDIR=$PKGCFGDIR uninstall
		else
			${MAKE} uninstall
		fi
	else	# not OSX
		if test -d /usr/lib64; then
			${MAKE} LIBDIRARCH=lib64 uninstall
		else
			${MAKE} uninstall
		fi
	fi
}

MAKE=make
if [ "$(uname)" == "SunOS" ]; then
	export MAKE=gmake
	export INSTALL_BIN=ginstall
	export CC=gcc
fi

if [[ "$(uname)" == *BSD* ]]; then
	export MAKE=gmake
	export PREFIX=/usr/local
fi

case "$1" in
  "" ) build;;
  "default" ) build;;
  "install" ) install;;
  "uninstall" ) uninstall;;
  "nix32" ) CFLAGS=-m32 LDFLAGS=-m32 build;;
  "cross-win32" ) CROSS=i686-w64-mingw32- build;;
  "cross-win64" ) CROSS=x86_64-w64-mingw32- build;;
  "cygwin-mingw32" ) CROSS=i686-pc-mingw32- build;;
  "cygwin-mingw64" ) CROSS=x86_64-w64-mingw32- build;;
  "clang" ) CC=clang build;;
  "gcc" ) CC=gcc build;;
  "ios" ) build_iOS;;
  "ios_armv7" ) build_iOS armv7;;
  "ios_armv7s" ) build_iOS armv7s;;
  "ios_arm64" ) build_iOS arm64;;
  * ) echo "Usage: make.sh [nix32|cross-win32|cross-win64|cygwin-mingw32|cygwin-mingw64|clang|gcc|ios|ios_armv7|ios_armv7s|ios_arm64|install|uninstall]"; exit 1;;
esac
