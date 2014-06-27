#!/bin/bash

BASE_DIR=$PWD
BUILD_DIR=${BASE_DIR}/MacOS
RELEASE_DIR=${BUILD_DIR}/release
FRAMEWORK_DIR=${RELEASE_DIR}/Frameworks
EXEC_NAME=OZMTool
FRAMEWORK=QtCore.framework

QT_VER=5
QTPATH=${QT_ENV}
QMAKE=${QTPATH}/bin/qmake
MAKE=make
OTOOL=otool
INSTALL_NAME_TOOL=install_name_tool
QT_LIB_DIR=${QTPATH}/lib
CONFIG_OPTION=release

if [ ! -d "${QT_ENV}" ]
then
  echo "QT_ENV variable is invalid! Please export it!"
  echo " ( e.g. export QT_ENV=/Users/username/Qt/5.3/clang_64 )"
  exit 1
fi

if [ `basename ${BASE_DIR}` != "OZMTool" ]
then
  echo "You are not executing from OZMTool directory!"
  exit 1
fi

# Lets begin
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
echo "Building..."
${QMAKE} -config ${CONFIG_OPTION} ${BASE_DIR}
${MAKE}
echo "Copying files..."
mkdir ${RELEASE_DIR}
cp ${BUILD_DIR}/${EXEC_NAME} ${RELEASE_DIR}
cp ${BASE_DIR}/README ${RELEASE_DIR}
mkdir ${FRAMEWORK_DIR}
cp -R ${QT_LIB_DIR}/${FRAMEWORK} ${FRAMEWORK_DIR}

echo "Setting up proper links..."
# Set identification name
${INSTALL_NAME_TOOL} -id @executable_path/Frameworks/${FRAMEWORK}/Versions/${QT_VER}/QtCore \
       ${FRAMEWORK_DIR}/${FRAMEWORK}/Versions/${QT_VER}/QtCore

# Set path
${INSTALL_NAME_TOOL} -change ${QT_LIB_DIR}/${FRAMEWORK}/Versions/${QT_VER}/QtCore \
        @executable_path/Frameworks/${FRAMEWORK}/Versions/${QT_VER}/QtCore \
        ${RELEASE_DIR}/${EXEC_NAME}

echo "Packing it up..."
cd ${RELEASE_DIR}
tar czvf ${BASE_DIR}/${EXEC_NAME}_vXX.tar.gz *

echo "Done!"
