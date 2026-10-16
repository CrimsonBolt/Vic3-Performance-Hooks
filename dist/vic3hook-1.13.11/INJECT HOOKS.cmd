@echo off
rem vic3hook: inject into the ALREADY RUNNING game. Start Victoria 3,
rem LOAD your save, PAUSE the game, then double-click this. The window
rem stays open so you can read the result either way. On launch the kit
rem checks GitHub for a newer version and ASKS: update now, open the
rem releases page, or keep the current one. Nothing is downloaded or
rem changed without an explicit choice, and your vic3hook.ini is
rem preserved. Pass -SkipUpdate to disable the check for that run.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0inject_running.ps1" %*
echo.
pause
