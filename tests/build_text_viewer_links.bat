@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\text_viewer_links" mkdir "%ROOT%\build\tests\text_viewer_links"
pushd "%ROOT%\build\tests\text_viewer_links" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_text_viewer_links.c" "%ROOT%\tests\text_viewer_links_unreached.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\initializers\markup_tags.c" "%ROOT%\src\core\text_markup.c" "%ROOT%\src\ui\controls\controls_std.c" /Fe:test_text_viewer_links.exe /link /OPT:REF user32.lib shell32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_text_viewer_links.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
