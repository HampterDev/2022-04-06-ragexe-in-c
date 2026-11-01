@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\emote_timing"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_emote_timing.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /std:c11 /W3 /WX /O2 /MD /I"%OUT%" "%ROOT%\tests\test_emote_timing.c" /Fe:test_emote_timing.exe
if errorlevel 1 (popd & exit /b 1)
test_emote_timing.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
