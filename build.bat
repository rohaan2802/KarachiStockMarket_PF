@echo off
call "%~dp0msvc_build.bat" "%~dp022i-2327_A_Project.cpp"
echo EXITCODE=%ERRORLEVEL%
exit /b %ERRORLEVEL%
