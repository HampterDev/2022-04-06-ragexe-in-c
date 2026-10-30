@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests" mkdir "%ROOT%\build\tests"
pushd "%ROOT%\build\tests"
for /L %%I in (1,1,18) do (
    cl /nologo /W1 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" /I"%ROOT%\tests" /DWINDOW_WRITER_OWNER=%%I /c "%ROOT%\tests\window_writer_owner.c" /Fo:test_window_writers_%%I.obj
    if errorlevel 1 (popd & exit /b 1) else if not errorlevel 0 (popd & exit /b 1)
)
cl /nologo /W1 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" /I"%ROOT%\tests" "%ROOT%\tests\test_window_writers.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\core\abstract_method.c" test_window_writers_1.obj test_window_writers_2.obj test_window_writers_3.obj test_window_writers_4.obj test_window_writers_5.obj test_window_writers_6.obj test_window_writers_7.obj test_window_writers_8.obj test_window_writers_9.obj test_window_writers_10.obj test_window_writers_11.obj test_window_writers_12.obj test_window_writers_13.obj test_window_writers_14.obj test_window_writers_15.obj test_window_writers_16.obj test_window_writers_17.obj test_window_writers_18.obj winmm.lib user32.lib "%ROOT%\src\std\list.c" /Fe:test_window_writers.exe /link /OPT:REF
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
if not "%~1"=="build-only" (
    test_window_writers.exe
    if errorlevel 1 (popd & exit /b 1) else if not errorlevel 0 (popd & exit /b 1)
)
popd
