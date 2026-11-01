@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\pet_accessory"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_pet_accessory.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
if not exist "%OUT%\runtime" mkdir "%OUT%\runtime"
pushd "%ROOT%\vendor" || exit /b 1
cl /nologo /c /O2 /MD /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS lua-5.1.4\src\*.c /Fo"%OUT%\runtime\\"
if errorlevel 1 (popd & exit /b 1)
type nul > "%OUT%\runtime\objects.rsp"
for %%s in (lua-5.1.4\src\*.c) do >> "%OUT%\runtime\objects.rsp" echo "%OUT%\runtime\%%~ns.obj"
lib /nologo /OUT:"%OUT%\lua_test.lib" @"%OUT%\runtime\objects.rsp"
if errorlevel 1 (popd & exit /b 1)
popd
pushd "%OUT%" || exit /b 1
cl /nologo /std:c11 /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%OUT%" "%ROOT%\tests\test_pet_accessory.c" lua_test.lib /Fe:test_pet_accessory.exe
if errorlevel 1 (popd & exit /b 1)
test_pet_accessory.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
