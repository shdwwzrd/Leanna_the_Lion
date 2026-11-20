@echo off
rem Builds the DEMO: build\leanna_demo.gb (copied to Leanna_Demo.gb). Same as build.bat but with -DDEMO:
rem the run ends after DEMO_LEVELS levels (default 3 = 45 seconds) with an "END OF DEMO" screen.
rem Use build.bat first (or any time art changed) so the converted tile data is current; this script converts too.
setlocal
if "%GBDK_HOME%"=="" set GBDK_HOME=C:\Users\shdwwzrd\Documents\gbdk-win64\gbdk
if not exist build mkdir build

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\convert_assets.ps1"
if errorlevel 1 exit /b 1

"%GBDK_HOME%\bin\lcc" -Wf-bo1 -c -o build\title_data.o src\title_data.c
if errorlevel 1 exit /b 1

"%GBDK_HOME%\bin\lcc" -DDEMO -Wm-yt0x03 -Wm-ya1 -Wm-yo2 -Wm-yn"LEANNADEMO" -o build\leanna_demo.gb src\main.c src\gfx_data.c build\title_data.o
if errorlevel 1 exit /b 1
copy /Y build\leanna_demo.gb "%~dp0Leanna_Demo.gb" > nul
if errorlevel 1 exit /b 1
echo Built build\leanna_demo.gb and copied it to Leanna_Demo.gb
