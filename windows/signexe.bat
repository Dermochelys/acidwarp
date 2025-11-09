 @echo off
setlocal enabledelayedexpansion

REM Find the latest version of signtool.exe
set "KITS_PATH=C:\Program Files (x86)\Windows Kits\10\bin"
set "SIGNTOOL="

REM List versions in reverse order (newest first) and find the first one with signtool.exe
for /f "delims=" %%v in ('dir /b /o-n "%KITS_PATH%"') do (
    if exist "%KITS_PATH%\%%v\x64\signtool.exe" (
        set "SIGNTOOL=%KITS_PATH%\%%v\x64\signtool.exe"
        goto :found
    )
)

:found
if not defined SIGNTOOL (
    echo Error: signtool.exe not found in Windows Kits directory
    exit /b 1
)

echo Using signtool: %SIGNTOOL%

"%SIGNTOOL%" sign /v /debug /fd SHA256 /tr "http://timestamp.acs.microsoft.com" /td SHA256 /dlib "%USERPROFILE%\AppData\Local\Microsoft\MicrosoftTrustedSigningClientTools\Azure.CodeSigning.Dlib.dll" /dmdf "metadata.json" .\acidwarp-windows.exe
