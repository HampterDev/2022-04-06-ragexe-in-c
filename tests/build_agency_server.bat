@echo off
setlocal
set "ROOT=%~dp0.."
if "%~1"=="" (echo Usage: build_agency_server.bat path-to-patched-Hercules & exit /b 1)
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\agency_server" mkdir "%ROOT%\build\tests\agency_server"
python "%ROOT%\tests\extract_agency_server.py" "%~1" "%ROOT%\build\tests\agency_server\agency_server_handler.inc" || exit /b 1
pushd "%ROOT%\build\tests\agency_server"
cl /nologo /W3 /WX /std:c11 /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I. "%ROOT%\tests\test_agency_server.c" /Fe:test_agency_server.exe
if errorlevel 1 (popd & exit /b 1)
test_agency_server.exe server-list.bin
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
