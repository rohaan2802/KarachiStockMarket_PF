@echo off
REM Launch the simulator in a real Windows console.
REM Cursor / VS Code integrated terminal often breaks _getch() and colors.
cd /d "%~dp0"

if not exist "KSE_Simulator.exe" (
  echo Building first...
  call build.bat
  if errorlevel 1 (
    echo Build failed. Install MSVC Build Tools, then re-run.
    pause
    exit /b 1
  )
)

if not exist "companies.txt" (
  echo ERROR: companies.txt missing in project folder.
  pause
  exit /b 1
)

echo Opening Karachi Stock Market Simulator in a new console window...
start "Karachi Stock Market PF" /D "%~dp0" cmd /k KSE_Simulator.exe
exit /b 0
