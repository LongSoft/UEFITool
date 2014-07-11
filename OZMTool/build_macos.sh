#!/bin/bash

BASE_DIR=$PWD
BUILD_DIR=${BASE_DIR}/MacOS
RELEASE_DIR=${BUILD_DIR}/release
FRAMEWORK_DIR=${RELEASE_DIR}/Frameworks
EXEC_NAME=OZMTool
FRAMEWORK_CORE=QtCore.framework
FRAMEWORK_XML=QtXml.framework
CORE_NAME=QtCore
XML_NAME=QtXml


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
cp -R ${QT_LIB_DIR}/${FRAMEWORK_CORE} ${FRAMEWORK_DIR}
cp -R ${QT_LIB_DIR}/${FRAMEWORK_XML} ${FRAMEWORK_DIR}

echo "Setting up proper links..."
# Set identification name
${INSTALL_NAME_TOOL} -id @executable_path/Frameworks/${FRAMEWORK_CORE}/Versions/${QT_VER}/${CORE_NAME} \
       ${FRAMEWORK_DIR}/${FRAMEWORK_CORE}/Versions/${QT_VER}/${CORE_NAME}

${INSTALL_NAME_TOOL} -id @executable_path/Frameworks/${FRAMEWORK_XML}/Versions/${QT_VER}/${XML_NAME} \
       ${FRAMEWORK_DIR}/${FRAMEWORK_XML}/Versions/${QT_VER}/${XML_NAME}

# Set path
${INSTALL_NAME_TOOL} -change ${QT_LIB_DIR}/${FRAMEWORK_CORE}/Versions/${QT_VER}/${CORE_NAME} \
        @executable_path/Frameworks/${FRAMEWORK_CORE}/Versions/${QT_VER}/${CORE_NAME} \
        ${RELEASE_DIR}/${EXEC_NAME}

${INSTALL_NAME_TOOL} -change ${QT_LIB_DIR}/${FRAMEWORK_XML}/Versions/${QT_VER}/${XML_NAME} \
        @executable_path/Frameworks/${FRAMEWORK_XML}/Versions/${QT_VER}/${XML_NAME} \
        ${RELEASE_DIR}/${EXEC_NAME}

${INSTALL_NAME_TOOL} -change ${QT_LIB_DIR}/${FRAMEWORK_CORE}/Versions/${QT_VER}/${CORE_NAME} \
        @executable_path/Frameworks/${FRAMEWORK_CORE}/Versions/${QT_VER}/${CORE_NAME} \
        ${FRAMEWORK_DIR}/${FRAMEWORK_XML}/Versions/${QT_VER}/${XML_NAME}

echo "Packing it up..."
cd ${RELEASE_DIR}
tar czvf ${BASE_DIR}/${EXEC_NAME}_vXX.tar.gz *

echo "Done!"
