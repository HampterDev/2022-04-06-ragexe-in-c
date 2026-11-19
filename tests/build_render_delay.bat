@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\render_delay" mkdir "%ROOT%\build\tests\render_delay"
pushd "%ROOT%\build\tests\render_delay" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_render_delay.c" "%ROOT%\src\std\render_delay.c" /Fe:test_render_delay.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_render_delay.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
