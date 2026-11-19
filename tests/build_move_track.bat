@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\move_track" mkdir "%ROOT%\build\tests\move_track"
pushd "%ROOT%\build\tests\move_track" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\tests\test_move_track.c" "%ROOT%\src\world\move_track.c" "%ROOT%\src\world\view_floors.c" "%ROOT%\src\body\grid_path.c" /Fe:test_move_track.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_move_track.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
