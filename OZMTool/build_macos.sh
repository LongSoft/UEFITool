#!/bin/bash

BASE_DIR=$PWD
BUILD_DIR=${BASE_DIR}/MacOS
RELEASE_DIR=${BUILD_DIR}/release
EXEC_NAME=OZMTool

QMAKE=qmake
MAKE=make
CONFIG_OPTION=release

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

echo "Packing it up..."
cd ${RELEASE_DIR}
zip -r ${BASE_DIR}/${EXEC_NAME}_vXX.zip *

echo "Done!"
