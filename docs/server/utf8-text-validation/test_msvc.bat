@echo off
REM Usage: test_msvc.bat <patched Hercules tree>
REM Builds and runs test_utf8_text.c against the patched tree with MSVC (x86, AddressSanitizer).
setlocal
set "TREE=%~1"
if "%TREE%"=="" (echo usage: test_msvc.bat ^<patched Hercules tree^> & exit /b 2)
set "OUT=%TEMP%\hercules-utf8-text-test"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul || exit /b 1
if not exist "%OUT%" mkdir "%OUT%"
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /Zi /Od /fsanitize=address /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE /DWIN32 /DPACKETVER=20220406 /I"%TREE%\src" /I"%TREE%\3rdparty" /I"%TREE%\3rdparty\msinttypes\include" "%~dp0test_utf8_text.c" /Fe:test_utf8_text.exe
if errorlevel 1 (popd & exit /b 1)
"%OUT%\test_utf8_text.exe"
set "RC=%errorlevel%"
popd
exit /b %RC%
