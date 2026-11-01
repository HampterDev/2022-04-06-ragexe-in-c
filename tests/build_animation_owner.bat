@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\animation" mkdir "%ROOT%\build\tests\animation"
pushd "%ROOT%\build\tests\animation" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_animation_owner.c" "%ROOT%\src\fileio\file.c" /Fe:test_animation_owner.exe /link /OPT:REF kernel32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_animation_owner.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W1 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_animation_drawing.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\ui\controls\window_draw.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\core\fatal.c" /Fe:test_animation_drawing.exe /link /OPT:REF kernel32.lib winmm.lib gdi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_animation_drawing.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
