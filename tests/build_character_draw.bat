@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%\src"
set "MODE=current"
set "REFERENCE="
set "EXTRA="
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\draw_owners_%MODE%" mkdir "%ROOT%\build\tests\draw_owners_%MODE%"
pushd "%ROOT%\build\tests\draw_owners_%MODE%" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%SOURCE%" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_character_draw.c" %EXTRA% "%SOURCE%\std\string.c" "%SOURCE%\std\container_memory.c" "%SOURCE%\std\vec.c" "%SOURCE%\core\once.c" "%SOURCE%\ui\character\character_std.c" "%SOURCE%\std\tree.c" /Fe:test_character_draw.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_character_draw.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
