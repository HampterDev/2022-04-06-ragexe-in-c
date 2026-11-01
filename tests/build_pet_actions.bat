@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\pet_actions"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_pet_actions.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /std:c11 /W3 /WX /O2 /MD /I"%OUT%" "%ROOT%\tests\test_pet_actions.c" /Fe:test_pet_actions.exe
if errorlevel 1 (popd & exit /b 1)
test_pet_actions.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
