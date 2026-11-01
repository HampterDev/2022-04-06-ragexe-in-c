@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\vendor\jpeg.lib" call "%ROOT%\vendor\build_vendor.bat"
if errorlevel 1 exit /b 1
if not exist "%ROOT%\build\tests\jpeg" mkdir "%ROOT%\build\tests\jpeg"
pushd "%ROOT%\build\tests\jpeg" || exit /b 1
set "FLAGS=/nologo /std:c11 /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src""
cl %FLAGS% "%ROOT%\tests\test_jpeg_codec.c" "%ROOT%\src\resources\jpeg_codec.c" "%ROOT%\src\resources\jpeg_storage.c" "%ROOT%\build\vendor\jpeg.lib" /Fe:test_jpeg_codec.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
test_jpeg_codec.exe "%ROOT%\tests\fixtures\jpeg_quadrants.jpg"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl %FLAGS% "%ROOT%\tests\test_jpeg_bitmap.c" /Fe:test_jpeg_bitmap.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
test_jpeg_bitmap.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl %FLAGS% "%ROOT%\tests\test_jpeg_capture.c" /Fe:test_jpeg_capture.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
test_jpeg_capture.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
