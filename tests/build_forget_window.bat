@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\forget_window" mkdir "%ROOT%\build\tests\forget_window"
pushd "%ROOT%\build\tests\forget_window" || exit /b 1
cl /nologo /W3 /WX /O2 /MD /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_forget_window.c" "%ROOT%\src\ui\wndmgr\forget_window.c" /Fe:test_forget_window.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_forget_window.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
