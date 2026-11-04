@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\byte_lines" mkdir "%ROOT%\build\tests\byte_lines"
pushd "%ROOT%\build\tests\byte_lines" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_byte_lines.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\ui\controls\text_lines.c" "%ROOT%\src\ui\controls\item_id_tag.c" "%ROOT%\src\core\text_array.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\core\text_format.c" "%ROOT%\src\core\hex.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_byte_lines.exe /link /OPT:REF user32.lib oleaut32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_byte_lines.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_balloon_text.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\core\text_util.c" "%ROOT%\src\ui\controls\controls_std.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\ui\std\ui_std_vector.c" "%ROOT%\src\ui\controls\text_lines.c" "%ROOT%\src\ui\controls\item_id_tag.c" "%ROOT%\src\core\text_format.c" "%ROOT%\src\core\hex.c" "%ROOT%\src\core\cstr.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_balloon_text.exe /link /OPT:REF user32.lib oleaut32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_balloon_text.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
