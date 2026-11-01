SET /P _inputversion= Please enter an version:

For %%A in ("%~1") do (
    Set Path=%%~pA
    Set Directory=%%~dA
    Set Folder=%%~dpA
    Set Name=%%~nA
)

DEL "%Folder%\release\%Name%_SOURCE_%_inputversion%.zip"

DEL "%Folder%\release\%Name%_%_inputversion%.hex"

"C:\Program Files (x86)\IZArc\IZARCC.exe" -a -cx "%Folder%%Name%_SOURCE_%_inputversion%.zip" "%~1"

COPY "%~1.leonardo.hex" "%Folder%\release\%Name%_%_inputversion%.hex"

MOVE "%Folder%%Name%_SOURCE_%_inputversion%.zip" "%Folder%\release\%Name%_SOURCE_%_inputversion%.zip"

REM create folder and copy all files to it

MKDIR C:\misc\neoretro\releases\%NAME%

"C:\Program Files (x86)\IZArc\IZARCC.exe" -a -cx "%Folder%%Name%_%_inputversion%.zip" "%Folder%release"

REM Clean up

DEL "%Folder%\release\%Name%_%_inputversion%.hex"
DEL "%Folder%\release\%Name%_SOURCE_%_inputversion%.zip"

MOVE "%Folder%%Name%_%_inputversion%.zip" "C:\misc\neoretro\releases\%Name%\%Name%_%_inputversion%.zip"

DEL "C:\misc\neoretro\releases\%Name%\%Name%_%_inputversion%.arduboy"
REN "C:\misc\neoretro\releases\%Name%\%Name%_%_inputversion%.zip" "%Name%_%_inputversion%.arduboy"


PAUSE





