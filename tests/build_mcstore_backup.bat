@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\mcstore_backup" mkdir "%ROOT%\build\tests\mcstore_backup"
pushd "%ROOT%\build\tests\mcstore_backup" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_mcstore_backup.c" "%ROOT%\src\contents\mcstore_json.c" "%ROOT%\src\json\service_json.c" "%ROOT%\src\json\json_tree.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\tests\json_test_support.c" "%ROOT%\vendor\yyjson-0.13.0\yyjson.c" /Fe:test_mcstore_backup.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_mcstore_backup.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
