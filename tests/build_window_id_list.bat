@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\window_id_list"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_window_id_list.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
python "%ROOT%\tests\extract_window_rows.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%OUT%" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_window_id_list.c" "%ROOT%\tests\window_id_list_unreached.c" "%ROOT%\src\ui\wndmgr\make_window.c" "%ROOT%\src\ui\wndmgr\window_table.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" /Fe:test_window_id_list.exe
if errorlevel 1 (popd & exit /b 1)
test_window_id_list.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
