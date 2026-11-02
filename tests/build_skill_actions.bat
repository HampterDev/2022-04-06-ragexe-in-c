@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\skill_actions" mkdir "%ROOT%\build\tests\skill_actions"
pushd "%ROOT%\build\tests\skill_actions" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_skill_action_registry.c" "%ROOT%\src\skills\skill_action_registry.c" "%ROOT%\src\skills\skill_action_mgr.c" "%ROOT%\src\skills\skill_actions.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\tree.c" /Fe:test_skill_action_registry.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_skill_action_registry.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
