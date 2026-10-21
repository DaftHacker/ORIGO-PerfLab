@echo off
setlocal
if "%~1"=="" (
  echo Usage: LaunchEditor.bat "C:\path\to\ORIGO_ENGINE.exe"
  exit /b 1
)
"%~1" "%~dp0ORIGO-PerfLab.origoproject"
endlocal
