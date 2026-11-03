@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\screenshot_save" mkdir "%ROOT%\build\tests\screenshot_save"
pushd "%ROOT%\build\tests\screenshot_save" || exit /b 1
python "%ROOT%\tests\extract_screenshot_save.py" "%ROOT%\src\world\mode_mgr.c" screenshot_save.inc
if errorlevel 1 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I. /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_screenshot_save.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\core\client_path.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_screenshot_save.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
test_screenshot_save.exe %*
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
