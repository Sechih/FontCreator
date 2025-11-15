@echo off
setlocal ENABLEDELAYEDEXPANSION

REM ----------------------------------------------------
REM  Path settings (for your project and Qt IFW)
REM ----------------------------------------------------

REM Project root (folder above installer)
set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."

REM CMake (you can replace with just "cmake" if it is in PATH)
set "CMAKE_EXE=C:\ST\STM32CubeCLT_1.15.0\CMake\bin\cmake.exe"

REM QtCreator build directory (Release)
set "BUILD_DIR=%ROOT_DIR%\build\Desktop_Qt_6_10_0_MinGW_64_bit-Release"

REM Where to put deploy (cmake --install)
set "DEPLOY_DIR=%ROOT_DIR%\deploy"

REM Qt Installer Framework (binarycreator)
set "QTIFW_BIN=C:\Tools\Qt\Tools\QtInstallerFramework\4.10\bin"
set "BINARYCREATOR=%QTIFW_BIN%\binarycreator.exe"

REM Installer config and packages
set "CONFIG_XML=%SCRIPT_DIR%config\config.xml"
set "PACKAGES_DIR=%SCRIPT_DIR%packages"
set "PACKAGE_ID=com.fontcreator.app"
set "PACKAGE_DATA_DIR=%PACKAGES_DIR%\%PACKAGE_ID%\data"

REM Output setup file name
set "OUTPUT_EXE=%ROOT_DIR%\FontCreator-1.0.0-setup.exe"

echo ==============================================
echo  FontCreator - installer build
echo  ROOT_DIR   = %ROOT_DIR%
echo  BUILD_DIR  = %BUILD_DIR%
echo  DEPLOY_DIR = %DEPLOY_DIR%
echo ==============================================
echo.

REM ----------------------------------------------------
REM  Check binarycreator and config.xml
REM ----------------------------------------------------
if not exist "%BINARYCREATOR%" (
    echo [ERROR] binarycreator.exe not found:
    echo         "%BINARYCREATOR%"
    echo Make sure Qt Installer Framework is installed
    echo and adjust QTIFW_BIN in make_installer.cmd if needed.
    exit /b 1
)

if not exist "%CONFIG_XML%" (
    echo [ERROR] config.xml not found:
    echo         "%CONFIG_XML%"
    exit /b 1
)

REM ----------------------------------------------------
REM  Step 1. Update deploy via cmake --install
REM ----------------------------------------------------
echo [1/3] Updating deploy via cmake --install ...

"%CMAKE_EXE%" --install "%BUILD_DIR%" --prefix "%DEPLOY_DIR%"
if errorlevel 1 (
    echo [ERROR] cmake --install failed.
    exit /b 1
)

echo [OK] deploy updated at "%DEPLOY_DIR%"
echo.

REM ----------------------------------------------------
REM  Step 2. Copy deploy to installer/packages/.../data
REM ----------------------------------------------------
echo [2/3] Updating package contents (data)...

REM Remove old data
if exist "%PACKAGE_DATA_DIR%" (
    echo    Removing old data folder ...
    rmdir /S /Q "%PACKAGE_DATA_DIR%"
)

mkdir "%PACKAGE_DATA_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to create "%PACKAGE_DATA_DIR%".
    exit /b 1
)

echo    Copying "%DEPLOY_DIR%\*" -> "%PACKAGE_DATA_DIR%\"
xcopy "%DEPLOY_DIR%\*" "%PACKAGE_DATA_DIR%\" /E /I /Y >nul
if errorlevel 1 (
    echo [ERROR] xcopy failed.
    exit /b 1
)

echo [OK] data updated.
echo.

REM ----------------------------------------------------
REM  Step 3. Run Qt Installer Framework (binarycreator)
REM ----------------------------------------------------
echo [3/3] Running Qt Installer Framework (binarycreator)...

"%BINARYCREATOR%" --offline-only -c "%CONFIG_XML%" -p "%PACKAGES_DIR%" "%OUTPUT_EXE%"
if errorlevel 1 (
    echo [ERROR] binarycreator failed.
    exit /b 1
)

echo.
echo [DONE] Installer created:
echo         "%OUTPUT_EXE%"
echo.
pause

endlocal
