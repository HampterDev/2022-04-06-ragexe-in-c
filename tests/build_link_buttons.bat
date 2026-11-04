@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "REFERENCE="
if not exist "%ROOT%\build\tests\link_buttons" mkdir "%ROOT%\build\tests\link_buttons"
pushd "%ROOT%\build\tests\link_buttons" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_link_buttons.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\ui\std\ui_std_vector.c" "%ROOT%\src\initializers\markup_tags.c" "%ROOT%\src\ui\chat\navi_text.c" /Fe:test_link_buttons.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_link_buttons.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
