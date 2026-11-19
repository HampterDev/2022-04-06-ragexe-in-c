@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\walk_steer" mkdir "%ROOT%\build\tests\walk_steer"
pushd "%ROOT%\build\tests\walk_steer" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_walk_steer.c" "%ROOT%\src\std\walk_steer.c" /Fe:test_walk_steer.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_walk_steer.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
