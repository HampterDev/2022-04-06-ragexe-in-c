@echo off
REM tests\oracle\build.bat - the oracle tests: the original executable mapped into the test process
REM and its pure routines called against the lifted C over their whole input space.
REM The original is %ORACLE_EXE%, by default ..\client-files\2022-04-06_Ragexe_1648707856.exe beside
REM this checkout. Every test is a 32-bit program linked away from the original's base 0x400000.
setlocal
set "ROOT=%~dp0..\.."
set "OUT=%ROOT%\build\tests\oracle"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%OUT%" mkdir "%OUT%"
if "%ORACLE_EXE%"=="" set "ORACLE_EXE=%ROOT%\..\client-files\2022-04-06_Ragexe_1648707856.exe"
if not exist "%ORACLE_EXE%" (echo oracle: original executable not found at "%ORACLE_EXE%"; set ORACLE_EXE & exit /b 1)
set "CFLAGS=/nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\tests\oracle" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include""
set "LFLAGS=/link /BASE:0x10000000 /FIXED /DYNAMICBASE:NO user32.lib"
pushd "%OUT%" || exit /b 1

cl %CFLAGS% "%ROOT%\tests\oracle\test_skill_poses.c" "%ROOT%\tests\oracle\oracle_image.c" "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\session\session_cast_effect.c" /Fe:test_skill_poses.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_skill_poses.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\job_lookups.inc" ^
  "src/session/session_job_rules.c::line::struct JobRange { WORD first, last; };" ^
  "src/session/session_job_rules.c::static int in_ranges(" ^
  "src/session/session_job_rules.c::int session_is_third_job(" ^
  "src/session/session_job_rules.c::BYTE session_is_fourth_job(" ^
  "src/session/predicates.c::BYTE session_is_expanded_super_novice_job(" ^
  "src/session/predicates.c::BYTE session_is_super_novice_job(" ^
  "src/session/predicates.c::BYTE session_is_expanded_job(" ^
  "src/session/predicates.c::BYTE session_is_doram_job(" ^
  "src/session/session_lookups.c::int session_remap_job(" ^
  "src/session/sprite_names.c::int session_get_mounted_job(" ^
  "src/session/session_party.c::DWORD session_get_party_color_by_seq(" ^
  "src/session/pet.c::const char *session_get_pet_act_text(" ^
  "src/session/session_misc.c::int session_get_max_job_level("
if errorlevel 1 (popd & exit /b 1)
cl %CFLAGS% /I"%OUT%" "%ROOT%\tests\oracle\test_job_lookups.c" "%ROOT%\tests\oracle\oracle_image.c" /Fe:test_job_lookups.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_job_lookups.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl %CFLAGS% "%ROOT%\tests\oracle\test_ground_skill.c" "%ROOT%\tests\oracle\oracle_image.c" "%ROOT%\src\actor\ground_skill.c" "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\effects\durations.c" /Fe:test_ground_skill.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_ground_skill.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl %CFLAGS% "%ROOT%\tests\oracle\test_skill_use.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\game\skill_use.c" "%ROOT%\src\game\skill_use_basic.c" "%ROOT%\src\game\skill_use_second.c" ^
  "%ROOT%\src\game\skill_use_extra.c" "%ROOT%\src\game\skill_use_third.c" "%ROOT%\src\game\skill_use_third2.c" ^
  "%ROOT%\src\game\skill_use_late.c" "%ROOT%\src\game\skill_use_fourth.c" "%ROOT%\src\game\skill_use_companion.c" ^
  "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\effects\durations.c" /Fe:test_skill_use.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_skill_use.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl %CFLAGS% "%ROOT%\tests\oracle\test_skill_hit.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\game\skill_notify.c" "%ROOT%\src\game\skill_hit_fx.c" "%ROOT%\src\render\math3d.c" ^
  "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\effects\durations.c" /Fe:test_skill_hit.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_skill_hit.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

popd
exit /b 0
