set BASE_DIR=%cd%
set BUILD_DIR=%BASE_DIR%\Win
set RELEASE_DIR=%BUILD_DIR%\release
set EXEC_NAME=OZMTool.exe

set GIT=git
set QMAKE=qmake
set MAKE=nmake
set CONFIG_OPTION=release
set OS_ID=win
set ZIP=7za
set PACKER=upx

for /f %%i in ('%GIT% describe') do set VERSION=%%i
set VERSION_HEADER=%BASE_DIR%\version.h

echo #ifndef VERSION_H > %VERSION_HEADER%
echo #define VERSION_H >> %VERSION_HEADER%
echo #define GIT_VERSION "%VERSION%" >> %VERSION_HEADER%
echo #endif // VERSION_H >> %VERSION_HEADER%

mkdir %BUILD_DIR%
cd %BUILD_DIR%
echo "Building..."
%QMAKE% -config %CONFIG_OPTION% %BASE_DIR%
%MAKE%
echo "Copying files..."
mkdir %RELEASE_DIR%
copy %BUILD_DIR%\%EXEC_NAME% %RELEASE_DIR%
copy %BASE_DIR%\README %RELEASE_DIR%

echo "Packing it up..."
cd %RELEASE_DIR%
%PACKER% -9 %EXEC_NAME%
%ZIP% a %BASE_DIR%\%EXEC_NAME%_%VERSION%_%OS_ID%.7z %EXEC_NAME% README

echo "Reverting version.h"
%GIT% checkout %VERSION_HEADER%
cd %BASE_DIR%
echo "Done!"
