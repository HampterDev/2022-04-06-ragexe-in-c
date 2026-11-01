@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\whisper_block" mkdir "%ROOT%\build\tests\whisper_block"
pushd "%ROOT%\build\tests\whisper_block" || exit /b 1
cl /nologo /W1 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_whisper_block.c" "%ROOT%\tests\whisper_command_support.c" "%ROOT%\tests\json_test_support.c" "%ROOT%\src\game\whisper_commands.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\json\service_json.c" "%ROOT%\src\json\json_tree.c" /Fe:test_whisper_block.exe /link /OPT:REF "%ROOT%\build\vendor\yyjson.lib" user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_whisper_block.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
