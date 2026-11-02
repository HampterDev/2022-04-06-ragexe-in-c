@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "DIR=json"
set "SANITIZE="
if "%~1"=="asan" (set "DIR=json_asan" & set "SANITIZE=/fsanitize=address /Z7")
if not exist "%ROOT%\build\tests\%DIR%" mkdir "%ROOT%\build\tests\%DIR%"
pushd "%ROOT%\build\tests\%DIR%" || exit /b 1
set "CFLAGS=%SANITIZE% /nologo /W3 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /DYYJSON_READER_DEPTH_LIMIT=256 /DYYJSON_WRITER_DEPTH_LIMIT=256"
cl %CFLAGS% /c /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\json\service_json.c" "%ROOT%\src\json\json_tree.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\tests\json_test_support.c" "%ROOT%\vendor\yyjson-0.13.0\yyjson.c"
if not "%errorlevel%"=="0" (popd & exit /b 1)
lib /nologo /out:json_text_storage.lib owned_text.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
set "COMMON=service_json.obj json_tree.obj string.obj container_memory.obj json_test_support.obj yyjson.obj json_text_storage.lib"
cl %CFLAGS% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_service_json.c" %COMMON% /Fe:test_service_json.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_service_json.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl %CFLAGS% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_mcstore_json.c" "%ROOT%\src\contents\mcstore_json.c" %COMMON% /Fe:test_mcstore_json.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_mcstore_json.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl %CFLAGS% /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_session_text.c" "%ROOT%\src\platform\win_utf8.c" %COMMON% /Fe:test_session_text.exe /link /OPT:REF advapi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_session_text.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl %CFLAGS% /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_json_text.c" json_tree.obj yyjson.obj /Fe:test_json_text.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_json_text.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
