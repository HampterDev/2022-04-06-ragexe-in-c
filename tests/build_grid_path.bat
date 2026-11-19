@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\grid_path" mkdir "%ROOT%\build\tests\grid_path"
pushd "%ROOT%\build\tests\grid_path" || exit /b 1
cl /nologo /W4 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_grid_path.c" "%ROOT%\src\body\grid_path.c" /Fe:test_grid_path.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_grid_path.exe "%ROOT%\tests\data\grid_path_vectors.txt"
set "CODE=%errorlevel%"
popd
exit /b %CODE%
