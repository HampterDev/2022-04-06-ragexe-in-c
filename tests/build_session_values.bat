@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%"
set "MODE=current"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\session_values_%MODE%" mkdir "%ROOT%\build\tests\session_values_%MODE%"
pushd "%ROOT%\build\tests\session_values_%MODE%" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /I"%SOURCE%\src" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_session_values.c" "%SOURCE%\src\session\snapshot_values.c" "%SOURCE%\src\std\string.c" "%SOURCE%\src\std\container_memory.c" /Fe:test_session_values.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_session_values.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
