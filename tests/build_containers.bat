@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\containers" mkdir "%ROOT%\build\tests\containers"
pushd "%ROOT%\build\tests\containers" || exit /b 1
cl /nologo /W4 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /I"%ROOT%\src" "%ROOT%\tests\test_containers.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" /Fe:test_containers.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_containers.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
