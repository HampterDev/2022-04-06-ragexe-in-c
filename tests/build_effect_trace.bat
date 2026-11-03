@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\effect_trace" mkdir "%ROOT%\build\tests\effect_trace"
pushd "%ROOT%\build\tests\effect_trace" || exit /b 1
set "R=%ROOT%\src\effects\rag"
set "DISPATCH="%R%\rag_effect_process.c" "%R%\rag_effect_process_491.c" "%R%\rag_effect_process_800.c""
set "LAUNCHERS="%R%\rag_skills_000_019.c" "%R%\rag_skills_020_039.c" "%R%\rag_skills_040_079.c" "%R%\rag_skills_080_119.c" "%R%\rag_skills_120_219.c" "%R%\rag_skills_220_259.c" "%R%\rag_skills_300_319.c" "%R%\rag_skills_320_339.c" "%R%\rag_skills_340_373.c" "%R%\rag_skills_374_419.c" "%R%\rag_skills_420_490.c" "%R%\rag_skills_491_559.c" "%R%\rag_skills_560_699.c" "%R%\rag_skills_700_799.c" "%R%\rag_skills_800_839.c" "%R%\rag_skills_840_879.c" "%R%\rag_skills_880_919.c" "%R%\rag_skills_920_2373.c" "%R%\rag_songs.c" "%R%\rag_msg_sprites.c" "%R%\rag_unreferenced.c" "%R%\rag_unreferenced_prims.c" "%R%\rag_process_moves.c""
set "CORE="%R%\rag_prim_launch.c" "%R%\effect_prim.c" "%R%\rag_effect.c" "%R%\rag_str_anim.c" "%R%\rag_str_playback.c" "%ROOT%\src\effects\str_path.c" "%ROOT%\src\effects\str_animation.c" "%ROOT%\src\effects\durations.c" "%ROOT%\src\effects\effect_objects.c" "%ROOT%\src\effects\effect_sound.c" "%ROOT%\src\actor\effect_launch.c" "%ROOT%\src\actor\position.c" "%ROOT%\src\effects\skills\ez2str_effect_files.c""
set "SUPPORT="%ROOT%\src\render\math3d.c" "%ROOT%\src\render\rp_project.c" "%ROOT%\src\session\session_effect_state.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c""
rem The client's compiler flags (build.bat), warnings as errors, and the rand() seam.
cl /nologo /std:c11 /O2 /Oy- /MD /GS /Gd /we4013 /W1 /WX /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /FI"%ROOT%\tests\effect_trace_rand.h" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\yyjson-0.13.0" %DISPATCH% %LAUNCHERS% %CORE% %SUPPORT% "%ROOT%\tests\test_effect_trace.c" /Fe:test_effect_trace.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_effect_trace.exe %*
set "CODE=%errorlevel%"
popd
exit /b %CODE%
