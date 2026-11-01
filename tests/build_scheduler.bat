@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\scheduler" mkdir "%ROOT%\build\tests\scheduler"
pushd "%ROOT%\build\tests\scheduler" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_scheduler.c" "%ROOT%\src\contents\ranking_style.c" "%ROOT%\src\std\string.c" "%ROOT%\src\initializers\markup_tags.c" /Fe:test_scheduler.exe /link /OPT:REF gdi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_scheduler.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
