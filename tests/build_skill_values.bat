@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%"
set "MODE=current"
set "REFERENCE="
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\skill_values_%MODE%" mkdir "%ROOT%\build\tests\skill_values_%MODE%"
pushd "%ROOT%\build\tests\skill_values_%MODE%" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%SOURCE%\src" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_map_nodes.c" "%SOURCE%\src\actor\actor_values.c" "%SOURCE%\src\world\world_std.c" "%SOURCE%\src\skills\player_skill_bundle.c" "%SOURCE%\src\skills\skill_info.c" "%SOURCE%\src\ui\std\ui_std_list.c" "%SOURCE%\src\ui\hotkeys\session_option_map.c" "%SOURCE%\src\ui\social\guild_containers.c" "%SOURCE%\src\contents\guild_lists.c" "%SOURCE%\src\std\container_memory.c" "%SOURCE%\src\std\string.c" "%SOURCE%\src\core\once_win32.c" "%ROOT%\tests\map_nodes_support.c" "%ROOT%\src\std\vec.c" "%SOURCE%\src\std\list.c" "%SOURCE%\src\std\tree.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_map_nodes.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_map_nodes.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
