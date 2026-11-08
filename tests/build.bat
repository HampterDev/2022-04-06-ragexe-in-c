@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\vendor\yyjson.lib" call "%ROOT%\vendor\build_vendor.bat"
if errorlevel 1 exit /b 1
if not exist "%ROOT%\build\tests" mkdir "%ROOT%\build\tests"
pushd "%ROOT%\build\tests"
cl /nologo /W1 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_job_hit_sounds.c" "%ROOT%\src\session\job_sounds.c" /Fe:test_job_hit_sounds.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_job_hit_sounds.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_mode_tooltip.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_rodex_send.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_rodex_contents.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_button_drawing.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_item_use.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_item_links.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_line_layout.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_tag_text_viewer.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_text_buttons.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_link_buttons.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_chat_navigation.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_readable_links.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_incoming_whisper.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_item_names.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_player_names.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_markup_helpers.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_navi_text.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_edit_navigation.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_text_buffer.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_edit_text.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_text_array.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_edit_history.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_multiline_storage.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_multiline_draw.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_multiline_lines.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_renderer_storage.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_model_resource_types.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_model_keyframes.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_rsx_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_language_input.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_text_markup.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_text_format.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_utf8.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_navi_text_lines.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_win_utf8.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_text_runs.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_text_render.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_byte_lines.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_plain_lines.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_shorten_text.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_fit_plain_text.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_item_tags.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_map_info.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_canvas_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_animation_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_rma_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_imf_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_sprite_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_texture_cache.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_surface_lifecycle.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_resource_lifecycle.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_bitmap.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_scheduler.bat"
if not "%errorlevel%"=="0" exit /b 1
call "%ROOT%\tests\build_ranking.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_cooldowns.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_talk_options.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_session_scalar.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_chat_tabs.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_session_fields.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_container_headers.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_mcstore_backup.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_user_hotkeys.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_whisper_block.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
call "%ROOT%\tests\build_owned_booking.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_agency_packets.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
call "%ROOT%\tests\build_lua_calls.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_navigation_objects.c" "%ROOT%\src\platform\win_utf8.c" "%ROOT%\tests\weather_state_support.c" "%ROOT%\src\navi\navi_containers.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\tests\navigation_object_support.c" "%ROOT%\src\platform\window_win32.c" "%ROOT%\src\core\utf8.c" /Fe:test_navigation_objects.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_navigation_objects.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_navigation_events.c" "%ROOT%\tests\navigation_event_support.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\navi\navi_window.c" "%ROOT%\src\navi\navi_records.c" "%ROOT%\src\ui\quests\navigation_view.c" "%ROOT%\src\game\navigation.c" "%ROOT%\src\navi\navi_object_queries.c" "%ROOT%\src\platform\clock_win32.c" /Fe:test_navigation_events.exe /link /OPT:REF user32.lib winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
test_navigation_events.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)

call "%~dp0build_name_cards.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_emblem_download.c" /Fe:test_emblem_download.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_emblem_download.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_ground_item.c" "%ROOT%\src\std\string.c" "%ROOT%\src\core\once_win32.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" winmm.lib "%ROOT%\src\platform\clock_win32.c" /Fe:test_ground_item.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_ground_item.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_own_states.c" /Fe:test_own_states.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_own_states.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)

call "%~dp0build_shortcut_packets.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_gm_name_packets.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_text_viewer_links.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_item_collection_toggle.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
if exist "%ROOT%\build\main_rebuild.exe" (
    call "%~dp0build_cps_imports.bat"
    if errorlevel 1 (popd & exit /b 1)
) else echo cps imports: skipped, build.bat has not built build\main_rebuild.exe

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_quest_value.c" "%ROOT%\src\contents\quest_value.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" "%ROOT%\src\ui\std\ui_std_vector.c" "%ROOT%\tests\quest_value_support.c" "%ROOT%\src\std\string_vec.c" /Fe:test_quest_value.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)

test_quest_value.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_quest_packets.c" "%ROOT%\src\contents\quest_value.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" "%ROOT%\src\ui\std\ui_std_vector.c" "%ROOT%\tests\quest_value_support.c" "%ROOT%\src\game\quest_list.c" "%ROOT%\src\game\quest_hunting.c" "%ROOT%\src\std\string_vec.c" "%ROOT%\src\platform\clock_win32.c" winmm.lib /Fe:test_quest_packets.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)

test_quest_packets.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)

cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_pathfinder_owner.c" "%ROOT%\tests\fixtures\pathfinder_reference.c" "%ROOT%\src\resources\attr_queries.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" "%ROOT%\src\core\numeric.c" "%ROOT%\src\render\math3d.c" "%ROOT%\tests\weather_state_support.c" "%ROOT%\src\std\tree.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_pathfinder_owner.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_pathfinder_owner.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_attribute_grid.c" "%ROOT%\src\resources\attr_queries.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_attribute_grid.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_attribute_grid.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_attribute_consumers.c" "%ROOT%\src\game\cells.c" "%ROOT%\src\game\cell_coordinates.c" "%ROOT%\src\resources\attr_queries.c" "%ROOT%\src\platform\clock_win32.c" /Fe:test_attribute_consumers.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_attribute_consumers.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_effect_durations.c" "%ROOT%\src\effects\durations.c" /Fe:test_effect_durations.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_effect_durations.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_hex.c" "%ROOT%\src\core\hex.c" /Fe:test_hex.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_hex.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_numeric.c" "%ROOT%\src\core\numeric.c" /Fe:test_numeric.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_numeric.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_cstr.c" "%ROOT%\src\core\cstr.c" /Fe:test_cstr.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_cstr.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /c /W3 /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\src\std\string.c" /Fo:string.obj
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_string_move.c" string.obj /Fe:test_string_move.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_string_move.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_once.c" "%ROOT%\src\core\once_win32.c" /Fe:test_once.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_once.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_message_tables.c" "%ROOT%\src\platform\win_utf8.c" "%ROOT%\src\iteminfo\item_loader.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\tests\message_tables_support.c" "%ROOT%\tests\weather_state_support.c" "%ROOT%\src\std\string.c" "%ROOT%\src\platform\window_win32.c" "%ROOT%\src\core\utf8.c" /Fe:test_message_tables.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_message_tables.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_math3d.c" "%ROOT%\src\render\math3d.c" /Fe:test_math3d.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_math3d.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_byte_queue.c" "%ROOT%\src\contents\byte_queue.c" /Fe:test_byte_queue.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_byte_queue.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_packets.c" "%ROOT%\src\contents\rag_connection.c" "%ROOT%\src\contents\byte_queue.c" "%ROOT%\src\core\once_win32.c" winmm.lib user32.lib "%ROOT%\src\platform\clock_win32.c" /Fe:test_packets.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_packets.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_session_companion.c" "%ROOT%\src\game\companion_packets.c" /Fe:test_session_companion.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_session_companion.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_skin_manager.c" "%ROOT%\src\ui\misc\skin_mgr.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string_vec.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_skin_manager.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_skin_manager.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
call "%~dp0build_character_model.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_character_draw.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_draw_maps.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_skill_values.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_traversal_callbacks.c" "%ROOT%\tests\model_names_fixture.c" "%ROOT%\tests\traversal_callbacks_support.c" "%ROOT%\src\world\world_near.c" "%ROOT%\src\world\model_actor.c" "%ROOT%\src\resources\model_res_queries.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\core\numeric.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" "%ROOT%\src\world\world_std.c" "%ROOT%\src\platform\clock_win32.c" winmm.lib /Fe:test_traversal_callbacks.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_traversal_callbacks.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
call "%~dp0build_skill_actions.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_gauge_sources.c" "%ROOT%\src\ui\character\gauge_value.c" "%ROOT%\src\ui\character\gauges.c" "%ROOT%\src\ui\character\bar_graph.c" "%ROOT%\src\ui\items\homun_bar.c" "%ROOT%\tests\gauge_sources_support.c" /Fe:test_gauge_sources.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_gauge_sources.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_game_context.c" "%ROOT%\src\game\context.c" "%ROOT%\src\world\mode_dispatch.c" "%ROOT%\src\world\mode_context.c" /Fe:test_game_context.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_game_context.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_world_context.c" "%ROOT%\src\world\view_context.c" /Fe:test_world_context.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_world_context.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_model_lighting.c" "%ROOT%\src\world\model_node_light.c" "%ROOT%\src\render\math3d.c" /Fe:test_model_lighting.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_model_lighting.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_effect_launch.c" "%ROOT%\src\actor\effect_launch.c" /Fe:test_effect_launch.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_effect_launch.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_status_icons.c" "%ROOT%\src\actor\player_status_icons.c" /Fe:test_status_icons.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_status_icons.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_world_player.c" "%ROOT%\src\world\world_player.c" /Fe:test_world_player.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_world_player.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_map_entry.c" "%ROOT%\src\game\map_entry.c" /Fe:test_map_entry.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_map_entry.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_cell_context.c" "%ROOT%\src\game\cell_context.c" /Fe:test_cell_context.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_cell_context.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_actor_appearance.c" "%ROOT%\src\actor\appearance.c" "%ROOT%\src\session\session_effect_state.c" /Fe:test_actor_appearance.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_actor_appearance.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_particle_view.c" "%ROOT%\src\effects\particle_view.c" /Fe:test_particle_view.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_particle_view.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_particle_process.c" "%ROOT%\src\effects\ez2str_playback.c" "%ROOT%\src\effects\str_animation.c" /Fe:test_particle_process.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_particle_process.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_particle_draw.c" "%ROOT%\src\effects\ez2str_draw.c" "%ROOT%\src\render\math3d.c" /Fe:test_particle_draw.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_particle_draw.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_effect_sound.c" "%ROOT%\src\effects\effect_sound.c" /Fe:test_effect_sound.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_effect_sound.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_gravity_control.c" "%ROOT%\tests\effect_base_stubs.c" /Fe:test_gravity_control.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_gravity_control.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_effect_targets.c" /Fe:test_effect_targets.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_effect_targets.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_effect_objects.c" "%ROOT%\src\effects\effect_objects.c" /Fe:test_effect_objects.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_effect_objects.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_prim_creation.c" "%ROOT%\src\effects\rag\rag_prim_launch.c" "%ROOT%\src\effects\effect_objects.c" /Fe:test_prim_creation.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_prim_creation.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_window_access.c" "%ROOT%\src\ui\window_access.c" /Fe:test_window_access.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_window_access.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_str_animation.c" "%ROOT%\src\effects\str_animation.c" "%ROOT%\src\effects\ez2str_playback.c" "%ROOT%\src\effects\rag\rag_str_playback.c" /Fe:test_str_animation.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_str_animation.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_actor_liveness.c" "%ROOT%\src\actor\clone_owner.c" /Fe:test_actor_liveness.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_actor_liveness.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_mode_messages.c" "%ROOT%\src\world\mode_dispatch.c" /Fe:test_mode_messages.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_mode_messages.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_mode_lifecycle.c" "%ROOT%\src\world\mode_loop.c" /Fe:test_mode_lifecycle.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )

test_mode_lifecycle.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_ai_context.c" "%ROOT%\src\platform\win_utf8.c" "%ROOT%\src\login\merc_ai.c" "%ROOT%\src\login\merc_ai_cmds.c" "%ROOT%\src\ui\hotkeys\session_option_map.c" "%ROOT%\src\core\numeric.c" "%ROOT%\src\std\tree.c" "%ROOT%\build\vendor\lua514.lib" winmm.lib user32.lib "%ROOT%\src\platform\clock_win32.c" "%ROOT%\src\platform\window_win32.c" "%ROOT%\src\core\utf8.c" /Fe:test_ai_context.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_ai_context.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_texture_quad.c" "%ROOT%\src\render\texture_quad.c" "%ROOT%\src\render\math3d.c" /Fe:test_texture_quad.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_texture_quad.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
call "%ROOT%\tests\build_memory.bat"
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
call "%ROOT%\tests\build_containers.bat"
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_broadcast_text.c" "%ROOT%\src\game\broadcast_text.c" "%ROOT%\src\core\utf8.c" /Fe:test_broadcast_text.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_broadcast_text.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_renew_quest_text.c" "%ROOT%\src\contents\renew_quest_text.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\std\string.c" /Fe:test_renew_quest_text.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_renew_quest_text.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_talk_type.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\session\session_talk_type.c" "%ROOT%\src\core\text_tokens.c" "%ROOT%\src\core\text_util.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string_vec.c" /Fe:test_talk_type.exe /link oleaut32.lib
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_talk_type.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_text_parsing.c" "%ROOT%\src\ui\controls\item_id_tag.c" "%ROOT%\src\ui\chat\speaker_name.c" "%ROOT%\src\game\scream_text.c" /Fe:test_text_parsing.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_text_parsing.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_text_tokens.c" "%ROOT%\src\core\text_tokens.c" "%ROOT%\src\std\string.c" "%ROOT%\src\ui\controls\sprite_paths.c" "%ROOT%\src\std\string_vec.c" "%ROOT%\src\std\vec.c" /Fe:test_text_tokens.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_text_tokens.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_c_text_streams.c" "%ROOT%\src\contents\guild_emblem_name.c" "%ROOT%\src\core\text_tokens.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string_vec.c" /Fe:test_c_text_streams.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_c_text_streams.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /c /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\libpng-1.6.36" /FI"%ROOT%\tests\png_alloc_fixture.h" "%ROOT%\src\resources\bitmap_png.c" /Fo:bitmap_png_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /c /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\png_alloc_fixture.h" "%ROOT%\src\resources\bitmap_storage.c" /Fo:bitmap_png_storage_test.obj
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_bitmap_png.c" bitmap_png_test.obj bitmap_png_storage_test.obj "%ROOT%\build\vendor\imglibs.lib" /Fe:test_bitmap_png.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_bitmap_png.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /c /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_password_encode.c" "%ROOT%\src\login\password_encode.c"
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /EHsc /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\vendor\crijndael\rijndael_api.cpp" test_password_encode.obj password_encode.obj "%ROOT%\build\vendor\crijndael.lib" /Fe:test_password_encode.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_password_encode.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
call "%~dp0build_failures.bat"
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
call "%~dp0build_character_preview.bat"
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /c /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\encoding_fixture.h" "%ROOT%\src\core\text_encoding.c" /Fo:text_encoding_test.obj
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_code_page_conversion.c" "%ROOT%\tests\encoding_fixture.c" text_encoding_test.obj "%ROOT%\src\platform\code_page_win32.c" /Fe:test_code_page_conversion.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_code_page_conversion.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /c /W3 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\encoding_fixture.h" "%ROOT%\src\ui\controls\ellipsize_text.c"
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_text_consumers.c" "%ROOT%\tests\encoding_fixture.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\std\string.c" "%ROOT%\src\initializers\markup_tags.c" ellipsize_text.obj text_encoding_test.obj user32.lib "%ROOT%\src\platform\code_page_win32.c" /Fe:test_text_consumers.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_text_consumers.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /EHsc /D_CRT_SECURE_NO_WARNINGS /DFONT_FAMILY_MOCK /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_font_family.cpp" "%ROOT%\vendor\font_backend\font_family.cpp" /Fe:test_font_family.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_font_family.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /EHsc /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_font_family.cpp" "%ROOT%\vendor\font_backend\font_family.cpp" gdiplus.lib /Fe:test_font_family_system.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_font_family_system.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
call "%ROOT%\tests\build_font_cache.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_login_pin_state.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
if not exist object_type_test mkdir object_type_test
cl /nologo /c /W4 /WX /O2 /MD /DTYPE_QUERY_FIXTURE /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\type_query_fixture.h" "%ROOT%\src\core\object_type.c" "%ROOT%\src\ui\controls\controls_object_types.c" "%ROOT%\src\ui\items\items_object_types.c" "%ROOT%\src\ui\character\character_object_types.c" "%ROOT%\src\ui\quests\quests_object_types.c" "%ROOT%\src\ui\social\social_object_types.c" "%ROOT%\src\ui\shops\shops_object_types.c" "%ROOT%\src\ui\chat\chat_object_types.c" "%ROOT%\src\actor\actor_object_types.c" "%ROOT%\src\resources\resource_object_types.c" "%ROOT%\src\ui\mail\mail_object_types.c" "%ROOT%\src\ui\hotkeys\hotkey_object_types.c" "%ROOT%\src\effects\skills\skill_object_types.c" "%ROOT%\src\ui\render\render_ui_object_types.c" "%ROOT%\src\ui\map\map_object_types.c" "%ROOT%\src\ui\worldmap\worldmap_object_types.c" "%ROOT%\src\core\object_type_registry.c" /Fo:object_type_test\
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_object_type.c" "%ROOT%\tests\fixtures\type_query_reference.c" object_type_test\*.obj /Fe:test_object_type.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_object_type.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /c /W3 /WX /O2 /Oy- /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\ui_invalidation_fixture.h" "%ROOT%\src\ui\uiwindow.c" /Fo:ui_invalidation_test.obj
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Oy- /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_ui_invalidation.c" ui_invalidation_test.obj user32.lib /Fe:test_ui_invalidation.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_ui_invalidation.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /c /W4 /WX /O2 /Oy- /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\crash_dump_fixture.h" "%ROOT%\src\platform\crash_dump.c" /Fo:crash_dump_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /O2 /Oy- /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_crash_dump.c" crash_dump_test.obj dbghelp.lib /Fe:test_crash_dump.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_crash_dump.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /c /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\crash_report_fixture.h" "%ROOT%\src\diag\crash_report.c" /Fo:crash_report_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_crash_report.c" crash_report_test.obj user32.lib /Fe:test_crash_report.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_crash_report.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /c /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\crash_dialog_fixture.h" "%ROOT%\src\ui\misc\crash_dialog.c" /Fo:crash_dialog_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_crash_dialog.c" crash_dialog_test.obj /Fe:test_crash_dialog.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_crash_dialog.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /c /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\mouse_fixture.h" "%ROOT%\src\contents\mouse.c" /Fo:mouse_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /c /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /FI"%ROOT%\tests\mouse_fixture.h" "%ROOT%\src\platform\window_win32.c" /Fo:mouse_window_test.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_mouse.c" "%ROOT%\src\platform\win_utf8.c" "%ROOT%\src\core\utf8.c" mouse_test.obj mouse_window_test.obj /Fe:test_mouse.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_mouse.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /MD /I"%ROOT%\src" "%ROOT%\tests\test_touch.c" "%ROOT%\src\core\touch.c" /Fe:test_touch.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_touch.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_item_info.c" "%ROOT%\src\iteminfo\item_info.c" "%ROOT%\src\iteminfo\item_list.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" /Fe:test_item_info.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_item_info.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_guild_state.c" "%ROOT%\src\contents\guild_info.c" "%ROOT%\src\contents\guild_lists.c" "%ROOT%\src\ui\social\guild_containers.c" "%ROOT%\src\ui\social\guild_requests.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" "%ROOT%\tests\guild_state_support.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\tree.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_guild_state.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_guild_state.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_pick_state.c" "%ROOT%\src\world\pick_node.c" "%ROOT%\tests\pick_state_support.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\list.c" /Fe:test_pick_state.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_pick_state.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_snap_state.c" "%ROOT%\src\ui\misc\snap_mgr.c" /Fe:test_snap_state.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_snap_state.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_weather_state.c" "%ROOT%\src\actor\weather.c" "%ROOT%\src\actor\weather_instance.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\tests\weather_state_support.c" /Fe:test_weather_state.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_weather_state.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_str_paths.c" "%ROOT%\src\effects\str_path.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\effects\ez2str_particle.c" "%ROOT%\src\effects\ez2str_playback.c" "%ROOT%\src\effects\str_animation.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\effects\rag\rag_str_playback.c" /Fe:test_str_paths.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_str_paths.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_schedule_ids.c" "%ROOT%\src\contents\schedule_ids.c" /Fe:test_schedule_ids.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_schedule_ids.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_display_devices.c" /Fe:test_display_devices.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_display_devices.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_resource_paths.c" "%ROOT%\src\ui\resource\resource_paths.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\core\text_util.c" "%ROOT%\tests\resource_paths_support.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_resource_paths.exe /link /OPT:REF oleaut32.lib
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_resource_paths.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /std:c11 /source-charset:utf-8 /execution-charset:.949 /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_sound_manager.c" "%ROOT%\src\platform\thread_win32.c" /Fe:test_sound_manager.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_sound_manager.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_item_definitions.c" "%ROOT%\src\platform\win_utf8.c" "%ROOT%\tests\item_info_support.c" "%ROOT%\tests\weather_state_support.c" "%ROOT%\src\std\vec.c" "%ROOT%\build\vendor\lua514.lib" "%ROOT%\src\platform\window_win32.c" "%ROOT%\src\core\utf8.c" /Fe:test_item_definitions.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_item_definitions.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_item_restrictions.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_world_view.c" "%ROOT%\src\ui\quests\world_view_display.c" "%ROOT%\build\vendor\lua514.lib" /Fe:test_world_view.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_world_view.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_navigation_state.c" "%ROOT%\tests\navigation_state_support.c" "%ROOT%\src\navi\navi_manager.c" "%ROOT%\src\navi\navi_instance.c" "%ROOT%\src\navi\navi_guide.c" "%ROOT%\src\navi\navi_records.c" "%ROOT%\src\navi\navi_containers.c" "%ROOT%\src\world\pathinfo.c" "%ROOT%\src\render\texture_image.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\tests\weather_state_support.c" "%ROOT%\src\navi\navi_object_queries.c" /Fe:test_navigation_state.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_navigation_state.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_kafra_services.c" "%ROOT%\build\vendor\lua514.lib" /Fe:test_kafra_services.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_kafra_services.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_path_info.c" "%ROOT%\src\world\pathfinder.c" "%ROOT%\src\core\numeric.c" "%ROOT%\src\render\math3d.c" /Fe:test_path_info.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_path_info.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_resource_registry.c" "%ROOT%\src\resources\resource_registry.c" /Fe:test_resource_registry.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_resource_registry.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_resource_cache.c" "%ROOT%\tests\resource_cache_support.c" "%ROOT%\src\fileio\resource_cache_store.c" "%ROOT%\src\fileio\res_cache.c" "%ROOT%\src\fileio\res_get.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\fileio\grf_pak.c" "%ROOT%\src\fileio\grf_mapfile.c" "%ROOT%\src\resources\resource_registry.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\render\cache_lock.c" "%ROOT%\src\platform\clock_win32.c" "%ROOT%\src\core\cstr.c" "%ROOT%\src\platform\filesystem_win32.c" "%ROOT%\src\platform\thread_win32.c" /Fe:test_resource_cache.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_resource_cache.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_resource_errors.c" "%ROOT%\src\fileio\resource_errors.c" "%ROOT%\src\core\cstr.c" "%ROOT%\src\platform\thread_win32.c" /Fe:test_resource_errors.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_resource_errors.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_startup_settings.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%ROOT%\tests\build_grf_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_file_io.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\fileio\file_mgr.c" "%ROOT%\src\platform\filesystem_win32.c" "%ROOT%\src\platform\system_memory_win32.c" /Fe:test_file_io.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_file_io.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_resource_spans.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_resource_spans.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_resource_spans.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_ground_water.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\resources\gnd.c" "%ROOT%\src\world\ground_water.c" "%ROOT%\src\world\ground_lifecycle.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string.c" "%ROOT%\tests\weather_state_support.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_ground_water.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_ground_water.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_ground_queries.c" "%ROOT%\src\resources\attr_queries.c" "%ROOT%\src\effects\rag\prim_bounce.c" "%ROOT%\src\render\math3d.c" /Fe:test_ground_queries.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_ground_queries.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_ground_lifecycle.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\resources\attr_queries.c" /Fe:test_ground_lifecycle.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_ground_lifecycle.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_world_resource.c" "%ROOT%\src\fileio\file.c" "%ROOT%\src\fileio\resource_path.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\platform\filesystem_win32.c" /Fe:test_world_resource.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_world_resource.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_scene_storage.c" "%ROOT%\src\world\ray_picker.c" "%ROOT%\src\world\scene_geometry.c" /Fe:test_scene_storage.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_scene_storage.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_scene_lifecycle.c" /Fe:test_scene_lifecycle.exe /link /OPT:REF
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
test_scene_lifecycle.exe
if not "%errorlevel%"=="0" ( popd & exit /b 1 )
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_scene_bounds.c" "%ROOT%\src\world\scene_bounds.c" /Fe:test_scene_bounds.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_scene_bounds.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_world_models.c" "%ROOT%\src\platform\thread_win32.c" /Fe:test_world_models.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_world_models.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_model_load.c" "%ROOT%\src\world\model_load.c" /Fe:test_model_load.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_model_load.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_frustum.c" "%ROOT%\src\world\frustum.c" /Fe:test_frustum.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_frustum.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_camera_lifecycle.c" "%ROOT%\src\world\frustum.c" "%ROOT%\src\world\view_lifecycle.c" /Fe:test_camera_lifecycle.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_camera_lifecycle.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_camera_consumers.c" /Fe:test_camera_consumers.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_camera_consumers.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_actor_effect_cells.c" /Fe:test_actor_effect_cells.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_actor_effect_cells.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_jpeg.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_quest_time.c" "%ROOT%\src\platform\clock_win32.c" winmm.lib /Fe:test_quest_time.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)

test_quest_time.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_granny_playback.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_granny_models.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_granny_vertices.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_json.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_projection.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_forget_window.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_barter_records.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_barter_managers.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_render_operations.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_actor_actions.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_object_operations.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_named_client_dispatch.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_ui_drop.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_scroll_operations.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_list_operations.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_ui_extensions.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_effect_destruction.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_effect_operations.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_effect_factories.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_effect_trace.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_effect_data.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_particle_pools.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_ez2str_manager.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_content_managers.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_actor_helper_owners.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_quest_journal.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_guild_quest_owners.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_achievement_owner.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_town_info.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_lua_data_managers.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_skill_cast_chat.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_packet_dispatch.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_window_lists.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_startup_order.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_game_mode_layout.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_broadcast_packets.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_thread_text.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_attack_records.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
call "%~dp0build_pet_actions.bat"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" "%ROOT%\tests\test_xml.c" "%ROOT%\src\core\xml.c" "%ROOT%\src\std\string.c" "%ROOT%\src\core\memory.c" /Fe:test_xml.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_xml.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd

call "%~dp0build_emblem_cache.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_emblem_transport.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_local_account.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_local_character.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_local_shop.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_inventory_match.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_screenshot_save.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_map_enter_packet.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_status_wnd_info.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_basic_info_restore.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_effect_launch_duration.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_window_id_list.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_url_link.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_name_text.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_monster_skill_info.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_login_conn_entry.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_format_string_msg.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_private_airplane.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_perf_overlay.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_mercenary_job.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_heal_colours.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_meteor_storm_waves.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_airplane_window_slot.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_chat_room_title.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_latest_achievements.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_manager_alloc_failure.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_max_job_level.bat"
if not "%errorlevel%"=="0" exit /b 1

call "%~dp0build_gfx_batch.bat"
if not "%errorlevel%"=="0" exit /b 1

REM The library table (tools\libraries.txt): include check, and link check on build.bat's
REM release objects (built first when they are missing or older than src).
call "%~dp0..\tools\check_libraries.bat"
if not "%errorlevel%"=="0" exit /b 1
