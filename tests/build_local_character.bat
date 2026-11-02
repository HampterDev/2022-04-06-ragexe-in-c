@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\local_character" mkdir "%ROOT%\build\tests\local_character"
pushd "%ROOT%\build\tests\local_character" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_local_character.c" "%ROOT%\src\contents\character_settings.c" "%ROOT%\src\contents\settings_file.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\json\service_json.c" "%ROOT%\src\json\json_tree.c" "%ROOT%\src\platform\filesystem.c" /Fe:test_local_character.exe /link /OPT:REF "%ROOT%\build\vendor\yyjson.lib" user32.lib advapi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_local_character.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
