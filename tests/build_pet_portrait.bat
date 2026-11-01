@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\pet_portrait"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_pet_portrait.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /std:c11 /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%OUT%" "%ROOT%\tests\test_pet_portrait.c" /Fe:test_pet_portrait.exe
if errorlevel 1 (popd & exit /b 1)
test_pet_portrait.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
