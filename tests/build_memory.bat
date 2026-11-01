@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\memory" mkdir "%ROOT%\build\tests\memory"
pushd "%ROOT%\build\tests\memory" || exit /b 1
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_memory.c" /Fe:test_memory.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_memory.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_container_memory.c" "%ROOT%\tests\container_memory_mail_probe.c" "%ROOT%\tests\container_memory_vector_probe.c" "%ROOT%\tests\container_memory_ui_probe.c" "%ROOT%\tests\container_memory_controls_probe.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\vec.c" /Fe:test_container_memory.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_container_memory.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
