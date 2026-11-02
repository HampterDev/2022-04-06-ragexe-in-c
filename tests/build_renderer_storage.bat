@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\renderer" mkdir "%ROOT%\build\tests\renderer"
pushd "%ROOT%\build\tests\renderer" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_renderer_storage.c" "%ROOT%\src\std\std_sort.c" "%ROOT%\src\platform\clock.c" "%ROOT%\src\platform\system_memory.c" /Fe:test_renderer_storage.exe /link /OPT:REF winmm.lib user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_renderer_storage.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_renderer_storage.exe growth_failure >nul 2>&1
if not "%errorlevel%"=="3" (popd & exit /b 1)
test_renderer_storage.exe queue_failure >nul 2>&1
if not "%errorlevel%"=="3" (popd & exit /b 1)
echo Renderer exhaustion: growth and queue allocation failures terminate without returning invalid storage.
popd
exit /b 0
