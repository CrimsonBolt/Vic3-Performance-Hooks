@echo off
rem vic3hook: inject into the ALREADY RUNNING game. Start Victoria 3 first,
rem get to the main menu, then double-click this. The window stays open so
rem you can read the result either way.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0inject_running.ps1"
echo.
pause
