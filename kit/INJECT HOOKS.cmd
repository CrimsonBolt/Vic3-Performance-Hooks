@echo off
rem vic3hook: inject into the ALREADY RUNNING game. Start Victoria 3,
rem LOAD your save, PAUSE the game, then double-click this. The window
rem stays open so you can read the result either way. On launch the kit
rem checks GitHub for a newer version; a downloaded update is applied
rem below, after the injector exits (your vic3hook.ini is preserved).
rem Pass -SkipUpdate to disable the check for that run.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0inject_running.ps1" %*
if exist "%~dp0update\staged" powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0apply_update.ps1"
echo.
pause
