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
  "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\effects\durations.c" "%ROOT%\src\game\cell_coordinates.c" /Fe:test_skill_use.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_skill_use.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl %CFLAGS% "%ROOT%\tests\oracle\test_skill_hit.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\game\skill_notify.c" "%ROOT%\src\game\skill_hit_fx.c" "%ROOT%\src\render\math3d.c" ^
  "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\effects\durations.c" "%ROOT%\src\game\cell_coordinates.c" /Fe:test_skill_hit.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_skill_hit.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl %CFLAGS% "%ROOT%\tests\oracle\test_skill_cast.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\game\skill_cast.c" "%ROOT%\src\game\skill_cast_look.c" "%ROOT%\src\game\cell_coordinates.c" ^
  "%ROOT%\src\session\session_cast_effect.c" "%ROOT%\src\session\session_use_effect.c" "%ROOT%\src\effects\durations.c" ^
  "%ROOT%\src\render\math3d.c" "%ROOT%\src\std\tree.c" /Fe:test_skill_cast.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_skill_cast.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\str_path_open.inc" ^
  "src/effects/str_path.c::static int is_word_char(" ^
  "src/effects/str_path.c::static int split_str_path(" ^
  "src/effects/str_path.c::struct String *ez2str_min_path(" ^
  "src/effects/str_path.c::void ez2_str_particle_open("
if errorlevel 1 (popd & exit /b 1)
cl %CFLAGS% /I"%OUT%" "%ROOT%\tests\oracle\test_ez2str_files.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\effects\skills\ez2str_effect_files.c" "%ROOT%\src\std\string.c" /Fe:test_ez2str_files.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_ez2str_files.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\ez2str_head.inc" ^
  "src/effects/skills/ez2str_effect.c::line::#define HAT_FIRST 0x950" ^
  "src/effects/skills/ez2str_effect.c::line::#define ACTOR_RIDING(a)    ((const struct GameActor *)(a))->mounted" ^
  "src/effects/skills/ez2str_effect.c::line::struct TypeRange { short first, last; };" ^
  "src/effects/skills/ez2str_effect.c::static int type_in(" ^
  "src/effects/skills/ez2str_effect.c::line::#define TYPE_IN(set, type) type_in((set), (int)(sizeof(set) / sizeof((set)[0])), (type))" ^
  "src/effects/skills/ez2str_effect.c::static const struct TypeRange HEADGEAR_TYPES[] = {" ^
  "src/effects/skills/ez2str_effect.c::static const struct TypeRange HEAD_TYPES[] = {" ^
  "src/effects/skills/ez2str_effect.c::line::struct TypeShift { short first, last; signed char shift; };" ^
  "src/effects/skills/ez2str_effect.c::static const struct TypeShift HEAD_HEIGHT_SHIFTS[] = {" ^
  "src/effects/skills/ez2str_effect.c::static void lua_ask_hat(" ^
  "src/effects/skills/ez2str_effect.c::unsigned char ez2_str_effect_is_headgear_type(" ^
  "src/effects/skills/ez2str_effect.c::unsigned char ez2_str_effect_is_head_type(" ^
  "src/effects/skills/ez2str_effect.c::float ez2str_head_x_offset(" ^
  "src/effects/skills/ez2str_effect.c::int ez2str_head_height("
if errorlevel 1 (popd & exit /b 1)
cl %CFLAGS% /I"%OUT%" "%ROOT%\tests\oracle\test_ez2str_head.c" "%ROOT%\tests\oracle\oracle_image.c" /Fe:test_ez2str_head.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_ez2str_head.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\ez2str_init.inc" ^
  "src/effects/skills/ez2str_effect.c::line::#define HAT_FIRST 0x950" ^
  "src/effects/skills/ez2str_effect.c::line::#define PARTICLE_LIFETIME(p) ((p)->base.life_seconds)" ^
  "src/effects/skills/ez2str_effect.c::line::struct TypeRange { short first, last; };" ^
  "src/effects/skills/ez2str_effect.c::static int type_in(" ^
  "src/effects/skills/ez2str_effect.c::line::#define TYPE_IN(set, type) type_in((set), (int)(sizeof(set) / sizeof((set)[0])), (type))" ^
  "src/effects/skills/ez2str_effect.c::static const struct TypeRange HEADGEAR_TYPES[] = {" ^
  "src/effects/skills/ez2str_effect.c::static const struct TypeRange LOOPING_TYPES[] = {" ^
  "src/effects/skills/ez2str_effect.c::static const struct TypeRange KEYED_LOOPING_TYPES[] = {" ^
  "src/effects/skills/ez2str_effect.c::static const struct TypeRange BEFORE_ACTORS_TYPES[] = {" ^
  "src/effects/skills/ez2str_effect.c::static void lua_ask_hat(" ^
  "src/effects/skills/ez2str_effect.c::static void cue(" ^
  "src/effects/skills/ez2str_effect.c::static void cue_message(" ^
  "src/effects/skills/ez2str_effect.c::static void set_list(" ^
  "src/effects/skills/ez2str_effect.c::unsigned char ez2_str_effect_is_headgear_type(" ^
  "src/effects/skills/ez2str_effect.c::void ez2_str_effect_init(" ^
  "src/effects/skills/ez2str_effect.c::static double screen_angle_reversed(" ^
  "src/effects/skills/ez2str_effect.c::static void hat_render_order(" ^
  "src/effects/skills/ez2str_effect.c::line::#define WAVE_BEGIN_SPELL \"effect\\EF_BeginSpell.wav\"" ^
  "src/effects/skills/ez2str_effect.c::line::#define WAVE_BEGIN_SPELL_LC \"effect\\ef_beginspell.wav\"" ^
  "src/effects/skills/ez2str_effect.c::void ez2_str_effect_after_init("
if errorlevel 1 (popd & exit /b 1)
cl %CFLAGS% /I"%OUT%" "%ROOT%\tests\oracle\test_ez2str_init.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\effects\skills\ez2str_effect_files.c" "%ROOT%\src\effects\effect_math.c" "%ROOT%\src\render\math3d.c" ^
  "%ROOT%\src\effects\durations.c" "%ROOT%\src\std\string.c" /Fe:test_ez2str_init.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_ez2str_init.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\rag_effect_init.inc" ^
  "src/effects/rag/rag_str_anim.c::line::#define FULL_OR_MIN(full, min) (g_session1.map_rule_flags[0] == 0 ? (full) : (min))" ^
  "src/effects/rag/rag_str_anim.c::static int roll(" ^
  "src/effects/rag/rag_str_anim.c::void rag_effect_init("
if errorlevel 1 (popd & exit /b 1)
cl %CFLAGS% /I"%OUT%" "%ROOT%\tests\oracle\test_rag_effect_init.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\effects\durations.c" /Fe:test_rag_effect_init.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_rag_effect_init.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\rag_process.inc" ^
  "src/actor/position.c::static float facing_of(" ^
  "src/actor/position.c::float render_object_calc_dir(" ^
  "src/actor/position.c::void render_object_turn_toward(" ^
  "src/actor/position.c::float render_object_calc_ground_dist(" ^
  "src/actor/position.c::static float view_facing(" ^
  "src/actor/position.c::static int dir_rounded(" ^
  "src/actor/position.c::int render_object_camera_dir(" ^
  "src/actor/position.c::int game_object_dir_of(" ^
  "src/render/rp_project.c::float renderer_screen_scale(" ^
  "src/render/rp_project.c::float renderer_project_depth(" ^
  "src/effects/effect_sound.c::void effect_play_wave_at(" ^
  "src/actor/game_object.c::void render_object_set_argb(" ^
  "src/effects/rag/rag_effect.c::int rag_effect_get_duration(" ^
  "src/session/session_names.c::const char *session_get_attr_wave_name(" ^
  "src/skills/skill_info.c::line::static const char k_zeroSkillIdName[] = \"Zero Skill\";" ^
  "src/skills/skill_info.c::const char *skill_get_id_name("
if errorlevel 1 (popd & exit /b 1)
set RAG=
for %%f in ("%ROOT%\src\effects\rag\rag_skills_*.c" "%ROOT%\src\effects\rag\prim_*.c" "%ROOT%\src\effects\rag\effect_prim*.c") do call set RAG=%%RAG%% "%%f"
cl %CFLAGS% /wd4018 /I"%OUT%" "%ROOT%\tests\oracle\test_rag_effect_process.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\effects\rag\rag_effect_process.c" "%ROOT%\src\effects\rag\rag_effect_process_491.c" "%ROOT%\src\effects\rag\rag_effect_process_800.c" ^
  "%ROOT%\src\effects\rag\rag_process_moves.c" "%ROOT%\src\effects\rag\rag_songs.c" "%ROOT%\src\effects\rag\rag_msg_sprites.c" ^
  %RAG% "%ROOT%\src\effects\rag\rag_unreferenced.c" "%ROOT%\src\effects\rag\rag_unreferenced_prims.c" ^
  "%ROOT%\src\effects\durations.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" ^
  /Fe:test_rag_effect_process.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_rag_effect_process.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\packet_handlers.inc" ^
  "src/game/packet_data.c::void packet_decode_move_data("
if errorlevel 1 (popd & exit /b 1)
cl %CFLAGS% /I"%OUT%" "%ROOT%\tests\oracle\test_packet_handlers.c" "%ROOT%\tests\oracle\oracle_image.c" ^
  "%ROOT%\src\game\packets_actor.c" "%ROOT%\src\game\effect_notify.c" "%ROOT%\src\game\cell_coordinates.c" ^
  "%ROOT%\src\effects\durations.c" /Fe:test_packet_handlers.exe %LFLAGS%
if errorlevel 1 (popd & exit /b 1)
.\test_packet_handlers.exe "%ORACLE_EXE%"
if not "%errorlevel%"=="0" (popd & exit /b 1)

popd
exit /b 0
