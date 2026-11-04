@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\latest_achievements"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_latest_achievements.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%OUT%" /I"%ROOT%\vendor\lua-5.1.4\src" "%ROOT%\tests\test_latest_achievements.c" "%ROOT%\src\std\tree.c" /Fe:test_latest_achievements.exe
if errorlevel 1 (popd & exit /b 1)
test_latest_achievements.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
