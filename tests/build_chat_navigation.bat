@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\chat_navigation" mkdir "%ROOT%\build\tests\chat_navigation"
pushd "%ROOT%\build\tests\chat_navigation" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_chat_navigation.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\ui\std\ui_std_vector.c" "%ROOT%\src\initializers\markup_tags.c" "%ROOT%\src\ui\chat\navi_text.c" "%ROOT%\src\std\string_vec.c" /Fe:test_chat_navigation.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_chat_navigation.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
