@echo off
setlocal EnableExtensions
cd /d "%~dp0"

REM Always delete old exe + rebuild so the latest .cpp is what you run.
set "SRC=%~dp022i-2327_A_Project.cpp"
set "EXE=%~dp022i-2327_A_Project.exe"

if not exist "%SRC%" (
  echo [ERROR] Source not found: %SRC%
  pause
  exit /b 1
)

echo ============================================
echo  Karachi Stock Market - Clean Build ^& Run
echo ============================================
echo.

echo Cleaning previous build...
if exist "%EXE%" del /f /q "%EXE%"
if exist "%~dp0*.obj" del /f /q "%~dp0*.obj" 2>nul
if exist "%~dp022i-2327_A_Project.obj" del /f /q "%~dp022i-2327_A_Project.obj" 2>nul

echo Building latest sources...
call "%~dp0msvc_build.bat" "%SRC%"
if errorlevel 1 (
  echo.
  echo [ERROR] Build failed - cannot run.
  pause
  exit /b 1
)

if not exist "%EXE%" (
  echo.
  echo [ERROR] Expected exe not found after build:
  echo   %EXE%
  pause
  exit /b 1
)

echo.
echo [OK] Built: %EXE%
echo Starting in a new console window...
echo Working directory: %cd%
echo.
echo Controls: Enter=refresh  P=portfolio  A=buy  R=sell  M=add money  E=exit
echo.

start "Karachi Stock Market" /D "%~dp0" cmd /k ""%EXE%""

endlocal
exit /b 0
