@echo off
cd /d "%~dp0"
call msvc_run.bat "%~dp022i-2327_A_Project.cpp"
exit /b %ERRORLEVEL%
