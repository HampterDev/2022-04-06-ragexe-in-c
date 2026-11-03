@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OUT=%ROOT%\build\tests\forget_window"
if not exist "%OUT%" mkdir "%OUT%"
python "%ROOT%\tests\extract_window_rows.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%ROOT%\build\tests\forget_window" || exit /b 1
cl /nologo /W3 /WX /O2 /MD /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%OUT%" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_forget_window.c" "%ROOT%\src\ui\wndmgr\forget_window.c" "%ROOT%\src\ui\wndmgr\window_table.c" /Fe:test_forget_window.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_forget_window.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
