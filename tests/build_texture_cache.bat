@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\texture_cache" mkdir "%ROOT%\build\tests\texture_cache"
pushd "%ROOT%\build\tests\texture_cache" || exit /b 1
for %%F in (1 0) do (
 cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_texture_cache_owner.c" /Fe:test_texture_cache_owner_%%F.exe /link /OPT:REF
 if errorlevel 1 (popd & exit /b 1) else if not errorlevel 0 (popd & exit /b 1)
 test_texture_cache_owner_%%F.exe
 if errorlevel 1 (popd & exit /b 1) else if not errorlevel 0 (popd & exit /b 1)
)
popd
exit /b 0
