@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\view_floors" mkdir "%ROOT%\build\tests\view_floors"
pushd "%ROOT%\build\tests\view_floors" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_view_floors.c" "%ROOT%\src\world\view_floors.c" "%ROOT%\src\world\presence.c" "%ROOT%\src\world\act_track.c" "%ROOT%\src\world\move_track.c" "%ROOT%\src\world\record_wait.c" "%ROOT%\src\body\grid_path.c" /Fe:test_view_floors.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_view_floors.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
