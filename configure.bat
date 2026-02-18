@echo off
REM Configure for Windows with x64 (avoids ARM64 VCTargetsPath errors on ARM64 systems)
if not exist build mkdir build
cd build
cmake -A x64 ..
cd ..
echo.
echo To build: cd build ^&^& cmake --build . --config Release
