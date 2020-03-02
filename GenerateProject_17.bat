@ECHO OFF

PUSHD %~dp0

REM rd /s /q %~dp0\Build
REM md %~dp0\Build

if not exist "%~dp0/Tools/CMake" (
    "%~dp0/Tools/7z/7z.exe" x "%~dp0/Tools/CMake.zip" -o"%~dp0/Tools/CMake/" -y
)

SET CMAKE="%~dp0\Tools\CMake\bin\cmake.exe"
SET CMAKE_GENERATOR="Visual Studio 15 2017 Win64"
SET CMAKE_BINARY_DIR=Build

MKDIR %CMAKE_BINARY_DIR% 2>NUL
PUSHD %CMAKE_BINARY_DIR%

%CMAKE% -G %CMAKE_GENERATOR% -Wno-dev "%~dp0"

REM IF ERRORLEVEL 1 (
REM     PAUSE
REM ) ELSE (
REM     START OpenLight.sln
REM )

POPD
POPD