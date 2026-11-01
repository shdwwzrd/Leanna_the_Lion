For %%A in ("%~1") do (
    Set Path=%%~pA
    Set Directory=%%~dA
    Set Folder=%%~dpA
    Set Name=%%~nA
)
SET Name=%Name:_THE_LION_TA.ino.tinyscreen_p=%

DEL "%Folder%\release\LEANNA\%Name%.bin"

COPY "%~1" "%Folder%release\LEANNA\%Name%.bin"

PAUSE





