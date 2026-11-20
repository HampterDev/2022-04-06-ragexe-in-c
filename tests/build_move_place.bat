@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\move_place" mkdir "%ROOT%\build\tests\move_place"
pushd "%ROOT%\build\tests\move_place" || exit /b 1
REM The place and the track compile without /std:c11, as the client's 32-bit build may; the body
REM core, the reference the test holds the place against, as its own test builds it.
cl /nologo /c /W4 /WX /we4013 /O2 /MD /TC /std:c11 /I"%ROOT%\src" "%ROOT%\src\body\body.c" "%ROOT%\src\body\grid_path.c"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /we4013 /O2 /MD /I"%ROOT%\src" "%ROOT%\tests\test_move_place.c" "%ROOT%\src\world\move_place.c" "%ROOT%\src\world\move_track.c" "%ROOT%\src\world\view_floors.c" body.obj grid_path.obj /Fe:test_move_place.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_move_place.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
