@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\rodex_contents" mkdir "%ROOT%\build\tests\rodex_contents"
pushd "%ROOT%\build\tests\rodex_contents" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_rodex_contents.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\ui\std\ui_std_vector.c" "%ROOT%\src\ui\chat\navi_text.c" "%ROOT%\src\ui\chat\message_tags.c" "%ROOT%\src\initializers\markup_tags.c" "%ROOT%\src\std\list.c" /Fe:test_rodex_contents.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_rodex_contents.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
