@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\incoming_whisper" mkdir "%ROOT%\build\tests\incoming_whisper"
pushd "%ROOT%\build\tests\incoming_whisper" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_incoming_whisper.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\initializers\markup_tags.c" /Fe:test_incoming_whisper.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_incoming_whisper.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
