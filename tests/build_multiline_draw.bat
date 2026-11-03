@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\multiline_draw" mkdir "%ROOT%\build\tests\multiline_draw"
pushd "%ROOT%\build\tests\multiline_draw" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_multiline_draw.c" "%ROOT%\src\ui\controls\window_coloured_lines.c" "%ROOT%\src\ui\controls\edit3_lines.c" "%ROOT%\src\core\text_array.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\core\byte_array.c" "%ROOT%\src\ui\items\item_tag_buttons.c" "%ROOT%\src\platform\clock_win32.c" "%ROOT%\src\platform\window_win32.c" /Fe:test_multiline_draw.exe /link /OPT:REF user32.lib winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_multiline_draw.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
