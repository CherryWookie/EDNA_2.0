@echo off
setlocal

rem Get the directory this script lives in
set "PROJECT_DIR=%~dp0"
cd /d "%PROJECT_DIR%"

rem pyenv setup
set "PYENV_ROOT=%USERPROFILE%\.pyenv"
set "PATH=%PYENV_ROOT%\shims;%PYENV_ROOT%\bin;%PATH%"

rem Set Python version for this shell
pyenv shell 3.12.9
if errorlevel 1 (
    echo Failed to set pyenv Python version.
    exit /b 1
)

rem Load ESP-IDF v6.0
cd /d "%USERPROFILE%/esp/v6.0/esp-idf/export.bat" || exit /b 1
call export.bat || exit /b 1

rem Go back to project
cd /d "%PROJECT_DIR%" || exit /b 1

echo.
echo ======================
python --version
echo IDF_PATH: %IDF_PATH%
echo Project: %CD%
echo ESP-IDF environment ready.
echo Ready in %PROJECT_DIR%
echo ======================
echo.

endlocal