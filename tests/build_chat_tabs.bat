@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\chat_tabs" mkdir "%ROOT%\build\tests\chat_tabs"
if not exist "%ROOT%\build\tests\chat_tabs\runtime" mkdir "%ROOT%\build\tests\chat_tabs\runtime"
pushd "%ROOT%\vendor" || exit /b 1
cl /nologo /c /O2 /MD /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS lua-5.1.4\src\*.c /Fo"%ROOT%\build\tests\chat_tabs\runtime\\"
if not "%errorlevel%"=="0" (popd & exit /b 1)
type nul > "%ROOT%\build\tests\chat_tabs\runtime\objects.rsp"
for %%s in (lua-5.1.4\src\*.c) do >> "%ROOT%\build\tests\chat_tabs\runtime\objects.rsp" echo "%ROOT%\build\tests\chat_tabs\runtime\%%~ns.obj"
lib /nologo /OUT:"%ROOT%\build\tests\chat_tabs\lua_test.lib" @"%ROOT%\build\tests\chat_tabs\runtime\objects.rsp"
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
pushd "%ROOT%\build\tests\chat_tabs" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_chat_tabs.c" /Fe:test_chat_tabs.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_chat_tabs.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_chat_tabs_lua.c" lua_test.lib /Fe:test_chat_tabs_lua.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_chat_tabs_lua.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_chat_tabs_layout.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\session\chat_tabs.c" "%ROOT%\src\session\chat_tabs_layout.c" lua_test.lib /Fe:test_chat_tabs_layout.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_chat_tabs_layout.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
python "%ROOT%\tests\extract_chat_layout_destroy.py" "%ROOT%" "%CD%"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%CD%" "%ROOT%\tests\test_chat_layout_save.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\session\chat_tabs.c" /Fe:test_chat_layout_save.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_chat_layout_save.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
