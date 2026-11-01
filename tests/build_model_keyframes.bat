@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\model_keyframes" mkdir "%ROOT%\build\tests\model_keyframes"
pushd "%ROOT%\build\tests\model_keyframes" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_model_keyframes.c" "%ROOT%\src\render\keyframes.c" "%ROOT%\src\render\math3d.c" /Fe:test_model_keyframes.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_model_keyframes.exe "%ROOT%\tests\fixtures\model_keyframes.bin"
set "CODE=%errorlevel%"
popd
exit /b %CODE%
