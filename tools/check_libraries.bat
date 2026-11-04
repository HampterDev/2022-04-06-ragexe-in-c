@echo off
REM tools\check_libraries.bat [OBJDIR]
REM Checks the library table tools\libraries.txt (docs\purist-review.md, "Restructuring: library
REM table and checks"):
REM   1. include check: tools\depmap\check_includes.py over every file under src;
REM   2. link check: tools\depmap\check_links.py over the dumpbin listing of OBJDIR's objects,
REM      build\obj (build.bat's release build) by default. Without OBJDIR, build.bat runs first
REM      when an object is missing or older than a file under src.
REM Both fail on a use the table does not allow and on an exception that matches nothing.
REM tests\build.bat runs this; build.bat check runs it on the objects of the variant it built.
setlocal
for %%i in ("%~dp0..") do set "ROOT=%%~fi"
python "%ROOT%\tools\depmap\check_includes.py" --strict
if errorlevel 1 exit /b 1
set "OBJ=%~1"
if defined OBJ goto links
set "OBJ=%ROOT%\build\obj"
python "%ROOT%\tools\depmap\stale_objects.py" "%OBJ%"
if not errorlevel 1 goto links
echo check_libraries: building the release objects first ^(build\check_libraries-build.log^)
call "%ROOT%\build.bat" > "%ROOT%\build\check_libraries-build.log" 2>&1
if errorlevel 1 ( echo *** build.bat FAILED, see build\check_libraries-build.log *** & exit /b 1 )
:links
for %%o in ("%OBJ%") do set "SYM=%ROOT%\build\libraries_%%~nxo.sym"
call "%ROOT%\tools\depmap\symbols.bat" "%OBJ%" "%SYM%"
if errorlevel 1 exit /b 1
python "%ROOT%\tools\depmap\check_links.py" "%SYM%" --strict
if errorlevel 1 exit /b 1
exit /b 0
