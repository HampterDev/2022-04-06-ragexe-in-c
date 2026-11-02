@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\rodex_send" mkdir "%ROOT%\build\tests\rodex_send"
pushd "%ROOT%\build\tests\rodex_send" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_rodex_send.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" "%ROOT%\src\core\cstr.c" /Fe:test_rodex_send.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_rodex_send.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
