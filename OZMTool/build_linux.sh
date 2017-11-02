#!/bin/bash

BASE_DIR=$PWD
BUILD_DIR=${BASE_DIR}/Linux
RELEASE_DIR=${BUILD_DIR}/release
EXEC_NAME=OZMTool

GIT=git
QMAKE=qmake
MAKE=make
CONFIG_OPTION=release
OS_ID=linux
PACKER=upx

VERSION=`${GIT} describe`
VERSION_HEADER=${BASE_DIR}/version.h

if [ `basename ${BASE_DIR}` != "OZMTool" ]
then
  echo "You are not executing from OZMTool directory!"
  exit 1
fi

echo "#ifndef VERSION_H" > ${VERSION_HEADER}
echo "#define VERSION_H" >> ${VERSION_HEADER}
echo "" >> ${VERSION_HEADER}
echo "#define GIT_VERSION \"${VERSION}\"" >> ${VERSION_HEADER}
echo "" >> ${VERSION_HEADER}
echo "#endif // VERSION_H" >> ${VERSION_HEADER}

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
${PACKER} -9 ${EXEC_NAME}
tar czvf ${BASE_DIR}/${EXEC_NAME}_${VERSION}_${OS_ID}.tar.gz *

echo "Reverting version.h"
${GIT} checkout ${VERSION_HEADER}

echo "Done!"
