@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "REFERENCE="
if not exist "%ROOT%\build\tests\line_layout" mkdir "%ROOT%\build\tests\line_layout"
pushd "%ROOT%\build\tests\line_layout" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_line_layout.c" "%ROOT%\src\core\text_markup.c" "%ROOT%\src\core\text_format.c" "%ROOT%\src\ui\chat\message_tags.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\ui\std\ui_std_vector.c" "%ROOT%\src\initializers\markup_tags.c" "%ROOT%\src\platform\code_page.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_line_layout.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_line_layout.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
