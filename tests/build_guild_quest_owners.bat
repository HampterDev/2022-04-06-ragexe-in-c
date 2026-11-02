@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OWNER=%ROOT%\src"
set "REFERENCE="
set "EXTRA="
if not exist "%ROOT%\build\tests\guild_quest_owners" mkdir "%ROOT%\build\tests\guild_quest_owners"
pushd "%ROOT%\build\tests\guild_quest_owners" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /FI"%ROOT%\tests\guild_quest_owner_boundaries.h" /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_guild_quest_owners.c" "%ROOT%\src\std\vec.c" "%OWNER%\contents\guild_mgr.c" "%OWNER%\contents\guild_emblem_req.c" "%OWNER%\contents\guild_emblem_download.c" "%OWNER%\contents\renew_quest.c" "%OWNER%\contents\renew_quest_maps.c" "%OWNER%\contents\quest_value.c" "%OWNER%\std\std_comdats.c" "%OWNER%\std\string.c" "%OWNER%\std\container_memory.c" "%OWNER%\fileio\res_maps.c" "%OWNER%\ui\std\ui_std_vector.c" "%OWNER%\std\string_vec.c" "%OWNER%\std\tree.c" %EXTRA% /Fe:test_guild_quest_owners.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_guild_quest_owners.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
