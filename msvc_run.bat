@echo off
REM Build then open the .exe in a real Windows console (for _getch / colors).
REM Usage: msvc_run.bat "C:\path\to\file.cpp"
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%msvc_build.bat" "%~f1"
if errorlevel 1 (
  echo Build failed.
  exit /b 1
)

set "DIR=%~dp1"
set "BASE=%~n1"
set "EXE=%DIR%%BASE%.exe"

if not exist "%EXE%" (
  echo ERROR: Expected exe not found: %EXE%
  exit /b 1
)

echo Launching "%EXE%" in external console...
start "%BASE%" /D "%DIR%" cmd /k ""%EXE%""
exit /b 0
