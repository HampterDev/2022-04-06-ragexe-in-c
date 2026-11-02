@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\edit_text" mkdir "%ROOT%\build\tests\edit_text"
pushd "%ROOT%\build\tests\edit_text" || exit /b 1
cl /nologo /W1 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_edit_text.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\ui\controls\edit_text.c" "%ROOT%\src\std\string.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\core\fatal.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_edit_text.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_edit_text.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
