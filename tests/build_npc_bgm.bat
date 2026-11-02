@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%"
if not "%~1"=="" set "SOURCE=%~1"
set "OUT=%ROOT%\build\tests\npc_bgm"
if not "%~2"=="" set "OUT=%~2"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%OUT%" mkdir "%OUT%"
python "%ROOT%\tests\extract_npc_bgm.py" "%SOURCE%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /std:c11 /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /I"%OUT%" /I"%SOURCE%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_npc_bgm.c" "%SOURCE%\src\std\string.c" /Fe:test_npc_bgm.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_npc_bgm.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
