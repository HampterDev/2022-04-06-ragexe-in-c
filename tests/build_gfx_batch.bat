@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\gfx_batch" mkdir "%ROOT%\build\tests\gfx_batch"
pushd "%ROOT%\build\tests\gfx_batch" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" "%ROOT%\tests\test_gfx_batch.c" "%ROOT%\src\render\gfx_batch.c" /Fe:test_gfx_batch.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_gfx_batch.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
