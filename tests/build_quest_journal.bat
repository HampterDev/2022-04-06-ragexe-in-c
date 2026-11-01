@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\quest_journal" mkdir "%ROOT%\build\tests\quest_journal"
pushd "%ROOT%\build\tests\quest_journal" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_quest_journal.c" "%ROOT%\src\ui\quests\quest_journal_layout.c" /Fe:test_quest_journal.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
test_quest_journal.exe
if errorlevel 1 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_quest_loading.c" /Fe:test_quest_loading.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
test_quest_loading.exe
if errorlevel 1 (popd & exit /b 1)
popd
exit /b 0
