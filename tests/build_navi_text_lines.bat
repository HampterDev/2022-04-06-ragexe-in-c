@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\navi_text_lines" mkdir "%ROOT%\build\tests\navi_text_lines"
pushd "%ROOT%\build\tests\navi_text_lines" || exit /b 1
cl /nologo /W4 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" "%ROOT%\tests\test_navi_text_lines.c" "%ROOT%\src\navi\navi_text_lines.c" "%ROOT%\src\core\utf8.c" /Fe:test_navi_text_lines.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_navi_text_lines.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
