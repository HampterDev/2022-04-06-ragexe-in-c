@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\owned_booking" mkdir "%ROOT%\build\tests\owned_booking"
pushd "%ROOT%\build\tests\owned_booking"
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" "%ROOT%\tests\test_owned_booking.c" "%ROOT%\src\contents\party_booking_data.c" /Fe:test_owned_booking.exe
if errorlevel 1 (popd & exit /b 1)
test_owned_booking.exe
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
