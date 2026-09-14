@echo off
REM Build any C++ source with MSVC (loads vcvars first).
REM Usage: msvc_build.bat "C:\path\to\file.cpp"
setlocal EnableExtensions

if "%~1"=="" (
  echo Usage: msvc_build.bat ^<file.cpp^>
  exit /b 1
)

set "SRC=%~f1"
set "DIR=%~dp1"
set "BASE=%~n1"

if not exist "%SRC%" (
  echo ERROR: Source not found: %SRC%
  exit /b 1
)

cd /d "%DIR%"

set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined VCVARS if exist "C:\Program Files\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined VCVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined VCVARS if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)
if not defined VCVARS if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
  set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

if not defined VCVARS (
  echo ERROR: vcvars64.bat not found. Install Visual Studio C++ Build Tools.
  exit /b 1
)

call "%VCVARS%" >nul
if errorlevel 1 (
  echo ERROR: Failed to initialize MSVC environment.
  exit /b 1
)

echo Building "%SRC%" ...
cl /EHsc /W3 /nologo /Fe:"%BASE%.exe" "%SRC%" /link user32.lib
set "EC=%ERRORLEVEL%"
echo EXITCODE=%EC%
exit /b %EC%
