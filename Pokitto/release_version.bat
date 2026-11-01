SET /P _inputversion= Please enter an version:

For %%A in ("%~1") do (
    Set Path=%%~pA
    Set Directory=%%~dA
    Set Folder=%%~dpA
    Set Name=%%~nA
)
SET Finputversion=V%_inputversion:.=_%

REM create folder and copy all files to it

MKDIR C:\misc\neoretro\releases\%NAME%_PK

COPY "%~1" "%Folder%\release\%Name%.bin"

COPY "%Folder%info.json" "%Folder%release\info.json"

@echo off
@echo "version": "%_inputversion%",>> %Folder%release\info.json
@echo "updated": "%date:~10,4%-%date:~4,2%-%date:~7,2%"}>> %Folder%release\info.json
@echo on

REM build release zip

"C:\Program Files (x86)\IZArc\IZARCC.exe" -a -cx "%Folder%%Name%_PK_%Finputversion%.zip" "%Folder%release\"

MOVE "%Folder%%Name%_PK_%Finputversion%.zip" "C:\misc\neoretro\releases\%Name%_PK\%Name%_PK_%Finputversion%.zip"




REM Clean up

DEL "%Folder%release\info.json"
DEL "%Folder%release\%Name%.bin"
DEL "C:\misc\neoretro\releases\%Name%_PK\%Name%_PK_%Finputversion%.pokitto"

REN "C:\misc\neoretro\releases\%Name%_PK\%Name%_PK_%Finputversion%.zip" "%Name%_PK_%Finputversion%.pokitto"

PAUSE





