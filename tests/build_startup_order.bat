@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\startup_order"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_startup_order.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%OUT%" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_startup_order.c" "%ROOT%\src\core\client_managers.c" "%ROOT%\src\core\manager_table.c" "%ROOT%\src\std\string.c" /Fe:test_startup_order.exe
if errorlevel 1 (popd & exit /b 1)
test_startup_order.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
