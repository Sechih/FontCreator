@echo off
setlocal
set BUILD_DIR=%~dp0build\Desktop_Qt_6_10_0_MinGW_64_bit-Release
set PREFIX=%~dp0deploy

"C:\ST\STM32CubeCLT_1.15.0\CMake\bin\cmake.exe" --install "%BUILD_DIR%" --prefix "%PREFIX%"
echo.
echo Deploy done: %PREFIX%
pause
