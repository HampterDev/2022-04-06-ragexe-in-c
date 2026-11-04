@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\emblem_cache" mkdir "%ROOT%\build\tests\emblem_cache"
pushd "%ROOT%\build\tests\emblem_cache" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" /FI"%ROOT%\tests\emblem_cache_boundaries.h" "%ROOT%\tests\test_emblem_cache.c" "%ROOT%\src\session\session_state.c" "%ROOT%\src\contents\guild_emblem_name.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\list.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\tree.c" winmm.lib /Fe:test_emblem_cache.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_emblem_cache.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
