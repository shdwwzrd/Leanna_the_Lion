@echo off
rem Builds build\leanna.gb with GBDK-2020. Set GBDK_HOME to override the default location.
rem The art in gfx\ is converted to tile data first, so edits to the PNGs are picked up on every build.
setlocal
if "%GBDK_HOME%"=="" set GBDK_HOME=C:\Users\shado\OneDrive\Documents\gbdk-win64\gbdk
if not exist build mkdir build

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\convert_assets.ps1"
if errorlevel 1 exit /b 1

rem The title picture data does not fit next to the code in ROM bank 0: it is built into bank 1.
"%GBDK_HOME%\bin\lcc" -Wf-bo1 -c -o build\title_data.o src\title_data.c
if errorlevel 1 exit /b 1

rem MBC1 + RAM + battery (1 RAM bank for the high score), 2 ROM banks
"%GBDK_HOME%\bin\lcc" -Wm-yt0x03 -Wm-ya1 -Wm-yo2 -Wm-yn"LEANNALION" -o build\leanna.gb src\main.c src\gfx_data.c build\title_data.o
if errorlevel 1 exit /b 1
rem a successful build: put a copy next to the build folder
copy /Y build\leanna.gb "%~dp0Leanna.gb" > nul
if errorlevel 1 exit /b 1
echo Built build\leanna.gb and copied it to Leanna.gb
