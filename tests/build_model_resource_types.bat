@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\model_resource_types" mkdir "%ROOT%\build\tests\model_resource_types"
pushd "%ROOT%\build\tests\model_resource_types" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_model_resource_types.c" "%ROOT%\src\resources\model_res_queries.c" "%ROOT%\src\core\owned_text.c" /Fe:test_model_resource_types.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_model_resource_types.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_model_node_lifetime.c" "%ROOT%\tests\model_node_lifetime_support.c" "%ROOT%\src\render\math3d.c" /Fe:test_model_node_lifetime.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_model_node_lifetime.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_model_mesh.c" "%ROOT%\src\std\std_sort.c" "%ROOT%\src\render\math3d.c" /Fe:test_model_mesh.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_model_mesh.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_model_mesh_normals.c" "%ROOT%\src\render\anim_mesh.c" "%ROOT%\src\render\math3d.c" /Fe:test_model_mesh_normals.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_model_mesh_normals.exe "%ROOT%\tests\fixtures\model_mesh_normals.bin"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_model_mesh_load.c" "%ROOT%\tests\model_mesh_load_support.c" "%ROOT%\src\resources\model_res_queries.c" "%ROOT%\src\std\std_sort.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\core\numeric.c" "%ROOT%\src\platform\clock_win32.c" winmm.lib /Fe:test_model_mesh_load.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_model_mesh_load.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
