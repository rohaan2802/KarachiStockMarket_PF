@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d "c:\Users\CodeTech\Desktop\cursor working space\KarachiStockMarket_PF"
cl /EHsc /W3 /Fe:KSE_Simulator.exe 22i-2327_A_Project.cpp /link user32.lib
echo EXITCODE=%ERRORLEVEL%
