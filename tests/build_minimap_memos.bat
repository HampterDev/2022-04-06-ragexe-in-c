@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\minimap_memos" mkdir "%ROOT%\build\tests\minimap_memos"
pushd "%ROOT%\build\tests\minimap_memos" || exit /b 1
python "%ROOT%\tests\extract_minimap_memos.py" "%ROOT%\src\ui\map\minimap_wnd.c" minimap_memos.inc
if errorlevel 1 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I. /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_minimap_memos.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\vec.c" /Fe:test_minimap_memos.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
test_minimap_memos.exe
if errorlevel 1 (popd & exit /b 1)
popd
exit /b 0
