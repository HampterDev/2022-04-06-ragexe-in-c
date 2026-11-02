@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\readable_links" mkdir "%ROOT%\build\tests\readable_links"
pushd "%ROOT%\build\tests\readable_links" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_readable_links.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\iteminfo\item_link.c" "%ROOT%\src\ui\chat\chat_link_items.c" "%ROOT%\src\ui\chat\item_link_text.c" "%ROOT%\src\ui\chat\navi_text.c" "%ROOT%\src\core\text_markup.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" /Fe:test_readable_links.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_readable_links.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
