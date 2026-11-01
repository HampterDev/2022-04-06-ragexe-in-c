@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\ui_drop" mkdir "%ROOT%\build\tests\ui_drop"
pushd "%ROOT%\build\tests\ui_drop" || exit /b 1
for /L %%I in (1,1,10) do (
 cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /DUI_DROP_OWNER=%%I /c "%ROOT%\tests\ui_drop_owner.c" /Fo:ui_drop_%%I.obj
 if errorlevel 1 (popd & exit /b 1) else if not errorlevel 0 (popd & exit /b 1)
)
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_ui_drop.c" ui_drop_1.obj ui_drop_2.obj ui_drop_3.obj ui_drop_4.obj ui_drop_5.obj ui_drop_6.obj ui_drop_7.obj ui_drop_8.obj ui_drop_9.obj ui_drop_10.obj "%ROOT%\src\std\vec.c" "%ROOT%\src\core\fatal.c" "%ROOT%\src\std\string_vec.c" "%ROOT%\src\std\list.c" /Fe:test_ui_drop.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_ui_drop.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
