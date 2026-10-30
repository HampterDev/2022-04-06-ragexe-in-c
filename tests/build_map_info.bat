@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OUT=%ROOT%\build\tests\map_info"
if not exist "%OUT%\runtime" mkdir "%OUT%\runtime"
pushd "%ROOT%\vendor" || exit /b 1
cl /nologo /c /O2 /MD /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS lua-5.1.4\src\*.c /Fo"%OUT%\runtime\\"
if not "%errorlevel%"=="0" (popd & exit /b 1)
type nul > "%OUT%\runtime\objects.rsp"
for %%s in (lua-5.1.4\src\*.c) do >> "%OUT%\runtime\objects.rsp" echo "%OUT%\runtime\%%~ns.obj"
lib /nologo /OUT:"%OUT%\lua_test.lib" @"%OUT%\runtime\objects.rsp"
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_map_info_owner.c" lua_test.lib /Fe:test_map_info_owner.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_map_info_owner.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
