@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\act_track" mkdir "%ROOT%\build\tests\act_track"
pushd "%ROOT%\build\tests\act_track" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_act_track.c" "%ROOT%\src\world\act_track.c" "%ROOT%\src\world\view_floors.c" /Fe:test_act_track.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_act_track.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
