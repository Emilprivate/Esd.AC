@echo off
mkdir build
cd build
cmake -G "Visual Studio 18 2026" -A Win32 ..
cmake --build . --config Release
pause
