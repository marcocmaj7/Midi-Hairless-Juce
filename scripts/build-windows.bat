@echo off
setlocal EnableDelayedExpansion

REM ==============================================================
REM  JUCE/CMake Windows build helper
REM  - Configures (with CMake) and builds the project on Windows
REM  - Defaults: Release, x64, Visual Studio 17 2022, target ALL_BUILD
REM  - Usage: build-windows.bat [-c Debug|Release] [-a x64|Win32] [-t Target]
REM                            [-g "Generator"] [--clean] [--reconfigure] [-h]
REM ==============================================================

set "CONFIG=Release"
set "ARCH=x64"
set "GENERATOR=Visual Studio 17 2022"
set "TARGET=ALL_BUILD"
set "BUILD_DIR=build"
set "RECONFIGURE=0"
set "DO_CLEAN=0"
set "THREADS=%NUMBER_OF_PROCESSORS%"
set "PAUSE_AT_END=0"

REM Resolve repository root and ensure we always run from there
set "SCRIPT_DIR=%~dp0"
for %%I in ("%~dp0..") do set "REPO_ROOT=%%~fI"
pushd "%REPO_ROOT%"

if "%~1"=="" goto :after_parse

:parse
if "%~1"=="" goto :after_parse
if /I "%~1"=="-c" (
  if "%~2"=="" goto :usage
  set "CONFIG=%~2"
  shift & shift & goto :parse
)
if /I "%~1"=="-a" (
  if "%~2"=="" goto :usage
  set "ARCH=%~2"
  shift & shift & goto :parse
)
if /I "%~1"=="-g" (
  if "%~2"=="" goto :usage
  set "GENERATOR=%~2"
  shift & shift & goto :parse
)
if /I "%~1"=="-t" (
  if "%~2"=="" goto :usage
  set "TARGET=%~2"
  shift & shift & goto :parse
)
if /I "%~1"=="--clean" (
  set "DO_CLEAN=1"
  shift & goto :parse
)
if /I "%~1"=="--reconfigure" (
  set "RECONFIGURE=1"
  shift & goto :parse
)
if /I "%~1"=="--pause" (
  set "PAUSE_AT_END=1"
  shift & goto :parse
)
if /I "%~1"=="-h" goto :usage
echo [ERROR] Argomento sconosciuto: %~1
goto :usage

:after_parse
echo ==============================================================
echo Building with CMake
echo   Config     : %CONFIG%
echo   Arch       : %ARCH%
echo   Generator  : %GENERATOR%
echo   Target     : %TARGET%
echo   Build Dir  : %BUILD_DIR%
echo ==============================================================

REM Check cmake is available
where cmake >nul 2>&1
if errorlevel 1 (
  echo [ERROR] CMake non trovato nel PATH. Installa CMake e riprova.
  goto :die1
)

REM Optionally clean
if "%DO_CLEAN%"=="1" (
  if exist "%BUILD_DIR%" (
    echo Pulizia cartella build: "%BUILD_DIR%" ...
    rmdir /S /Q "%BUILD_DIR%" 2>nul
  )
)

REM Configure with CMake if requested or if cache is missing
set "NEED_CONFIGURE=0"
if "%RECONFIGURE%"=="1" set "NEED_CONFIGURE=1"
if not exist "%BUILD_DIR%\CMakeCache.txt" set "NEED_CONFIGURE=1"

REM If reconfigure is requested, drop CMake cache to avoid generator/arch mismatch
if "%RECONFIGURE%"=="1" if exist "%BUILD_DIR%" (
  REM Clear top-level cache
  if exist "%BUILD_DIR%\CMakeCache.txt" del /F /Q "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
  if exist "%BUILD_DIR%\CMakeFiles" rmdir /S /Q "%BUILD_DIR%\CMakeFiles" >nul 2>&1

  REM Clear any nested CMake caches (e.g., FetchContent subbuilds like juce)
  for /f "usebackq delims=" %%f in (`dir /s /b "%BUILD_DIR%\CMakeCache.txt" 2^>nul`) do (
    del /F /Q "%%f" >nul 2>&1
  )
  for /f "usebackq delims=" %%d in (`dir /s /b /ad "%BUILD_DIR%\CMakeFiles" 2^>nul`) do (
    rmdir /S /Q "%%d" >nul 2>&1
  )

  REM Clear common FetchContent folders that pin generator/arch
  if exist "%BUILD_DIR%\_deps\juce-subbuild" rmdir /S /Q "%BUILD_DIR%\_deps\juce-subbuild" >nul 2>&1
  if exist "%BUILD_DIR%\_deps\juce-build" rmdir /S /Q "%BUILD_DIR%\_deps\juce-build" >nul 2>&1
)

if "%NEED_CONFIGURE%"=="1" (
  echo.
  echo [CMake] Configurazione del progetto...
  cmake -S . -B "%BUILD_DIR%" -G "%GENERATOR%" -A %ARCH%
  if errorlevel 1 (
    echo [ERROR] Fallita la configurazione CMake.
    goto :die1
  )
)

echo.
echo [CMake] Compilazione in corso...
cmake --build "%BUILD_DIR%" --config %CONFIG% --target %TARGET% -- /m:%THREADS%
if errorlevel 1 (
  echo [ERROR] Build fallita.
  goto :die1
)

REM Try to copy artefact to dist folder if the main exe exists
REM Name of the produced executable (observed name includes spaces)
set "EXE_NAME=Hairless MIDI Serial Bridge.exe"
set "CANDIDATE1=%BUILD_DIR%\HairlessMidiSerial_artefacts\%CONFIG%\%EXE_NAME%"
set "DIST_DIR=dist\%CONFIG%"

set "FOUND_EXE="
if exist "%CANDIDATE1%" (
  set "FOUND_EXE=%CANDIDATE1%"
) else (
  for /f "usebackq delims=" %%f in (`dir /s /b "%BUILD_DIR%\%EXE_NAME%" 2^>nul`) do (
    set "FOUND_EXE=%%f"
  )
)

if defined FOUND_EXE (
  if not exist "dist" mkdir "dist" >nul 2>&1
  if not exist "%DIST_DIR%" mkdir "%DIST_DIR%" >nul 2>&1
  copy /Y "%FOUND_EXE%" "%DIST_DIR%\%EXE_NAME%" >nul
  echo Artefatto copiato in "%DIST_DIR%\%EXE_NAME%"
) else (
  echo Nota: eseguibile principale non trovato automaticamente. La build e' comunque riuscita.
)

echo.
echo [OK] Build completata con successo.
goto :die0

:usage
echo.
echo Utilizzo:
echo   build-windows.bat ^[-c Debug^|Release^] ^[-a x64^|Win32^] ^[-t Target^]
echo                      ^[-g "Visual Studio 17 2022"^] ^[--clean^] ^[--reconfigure^]
echo Opzioni:
echo   -c    Configurazione di build (Default: Release)
echo   -a    Architettura (x64 o Win32) (Default: x64)
echo   -t    Target da compilare (Default: ALL_BUILD)
echo   -g    CMake generator, es. "Visual Studio 17 2022" (Default)
echo   --clean        Elimina la cartella build prima di procedere
echo   --reconfigure  Forza la rigenerazione dei file di progetto CMake
echo   -h             Mostra questo aiuto
echo Esempi:
echo   build-windows.bat
echo   build-windows.bat -c Debug
echo   build-windows.bat -c Release -a x64 -t HairlessMidiSerial
echo   build-windows.bat -g "Visual Studio 17 2022" --reconfigure
exit /b 0

:die1
set "EXIT_CODE=1"
goto :finish

:die0
set "EXIT_CODE=0"
goto :finish

:finish
if "%PAUSE_AT_END%"=="1" pause
popd
exit /b %EXIT_CODE%
