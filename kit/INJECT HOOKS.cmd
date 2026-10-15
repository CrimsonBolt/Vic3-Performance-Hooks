@echo off
rem vic3hook: inject into the ALREADY RUNNING game. Start Victoria 3, LOAD
rem your save, PAUSE the game, then double-click this. The window stays open
rem so you can read the result either way.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0inject_running.ps1"
echo.
pause
