@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%\src"
set "OUT=%ROOT%\build\tests\item_restrictions"
if not "%~1"=="" set "SOURCE=%~1"
if not "%~2"=="" set "OUT=%~2"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%OUT%" mkdir "%OUT%"
python "%ROOT%\tests\extract_item_restrictions.py" "%SOURCE%\iteminfo\item_loader.c" "%OUT%\item_restrictions_under_test.c"
if not "%errorlevel%"=="0" exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /O2 /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%SOURCE%" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_item_restrictions.c" item_restrictions_under_test.c "%SOURCE%\std\vec.c" /Fe:test_item_restrictions.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_item_restrictions.exe %3
set "CODE=%errorlevel%"
popd
exit /b %CODE%
