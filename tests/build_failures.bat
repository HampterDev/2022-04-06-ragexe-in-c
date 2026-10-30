@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\failures" mkdir "%ROOT%\build\tests\failures"
pushd "%ROOT%\build\tests\failures" || exit /b 1
cl /nologo /W4 /WX /O2 /MD /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_fatal.c" "%ROOT%\src\core\fatal.c" "%ROOT%\src\core\abstract_method.c" /Fe:test_fatal.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_fatal.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
