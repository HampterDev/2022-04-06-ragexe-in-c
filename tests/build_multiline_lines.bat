@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\multiline_lines" mkdir "%ROOT%\build\tests\multiline_lines"
pushd "%ROOT%\build\tests\multiline_lines" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_multiline_lines.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\ui\controls\window_coloured_lines.c" "%ROOT%\src\ui\controls\edit3_lines.c" "%ROOT%\src\core\text_array.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\core\byte_array.c" "%ROOT%\src\std\string.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\core\fatal.c" /Fe:test_multiline_lines.exe /link /OPT:REF user32.lib winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_multiline_lines.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
