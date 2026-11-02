@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\talk_options" mkdir "%ROOT%\build\tests\talk_options"
if not exist "%ROOT%\build\tests\talk_options\runtime" mkdir "%ROOT%\build\tests\talk_options\runtime"
pushd "%ROOT%\vendor" || exit /b 1
cl /nologo /c /O2 /MD /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS lua-5.1.4\src\*.c /Fo"%ROOT%\build\tests\talk_options\runtime\\"
if not "%errorlevel%"=="0" (popd & exit /b 1)
type nul > "%ROOT%\build\tests\talk_options\runtime\objects.rsp"
for %%s in (lua-5.1.4\src\*.c) do >> "%ROOT%\build\tests\talk_options\runtime\objects.rsp" echo "%ROOT%\build\tests\talk_options\runtime\%%~ns.obj"
lib /nologo /OUT:"%ROOT%\build\tests\talk_options\lua_test.lib" @"%ROOT%\build\tests\talk_options\runtime\objects.rsp"
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
pushd "%ROOT%\build\tests\talk_options" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_talk_option_storage.c" /Fe:test_talk_option_storage.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_talk_option_storage.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_talk_option_lua.c" lua_test.lib /Fe:test_talk_option_lua.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_talk_option_lua.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
