@echo off
setlocal

if not exist build mkdir build

cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
if errorlevel 1 goto :error

cmake --build build --config Release
if errorlevel 1 goto :error

echo Build succeeded.
exit /b 0

:error
echo Build failed.
exit /b %errorlevel%
