@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\item_tags" mkdir "%ROOT%\build\tests\item_tags"
pushd "%ROOT%\build\tests\item_tags" || exit /b 1
cl /nologo /W4 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_item_tag_list.c" /Fe:test_item_tag_list.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_item_tag_list.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
set "CFLAGS=/nologo /W1 /WX /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949"
cl %CFLAGS% /c /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\src\std\string.c" "%ROOT%\src\ui\items\item_tag_text.c" "%ROOT%\src\ui\items\item_tags_chat.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\ui\std\ui_std_vector.c"
if not "%errorlevel%"=="0" (popd & exit /b 1)
lib /nologo /OUT:item_tag_support.lib string.obj item_tag_text.obj item_tags_chat.obj std_comdats.obj container_memory.obj ui_std_vector.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl %CFLAGS% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_item_tags.c" /Fe:test_item_tags.exe /link /OPT:REF item_tag_support.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_item_tags.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
