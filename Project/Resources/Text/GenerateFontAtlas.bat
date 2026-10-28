@echo off
rem Resources/Text/Source 内の全フォント(.ttf/.otf)から MSDF アトラス(PNG + JSON)を Resources/Text/Generated に生成する
chcp 65001 > nul
setlocal

set "TEXT_DIR=%~dp0"
set "SOURCE_DIR=%TEXT_DIR%Source\"
set "OUTPUT_DIR=%TEXT_DIR%Generated\"
set "ATLAS_GEN=%TEXT_DIR%..\..\Externals\msdf-atlas-gen\msdf-atlas-gen.exe"
set "CHARSET=%TEXT_DIR%charset.txt"

if not exist "%ATLAS_GEN%" (
    echo [Error] msdf-atlas-gen.exe が見つかりません: "%ATLAS_GEN%"
    exit /b 1
)
if not exist "%SOURCE_DIR%" (
    echo [Error] Source フォルダが見つかりません: "%SOURCE_DIR%"
    exit /b 1
)
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

set "RESULT=0"
for %%F in ("%SOURCE_DIR%*.ttf" "%SOURCE_DIR%*.otf") do (
    echo [Generate] %%~nxF
    "%ATLAS_GEN%" -font "%%~fF" -charset "%CHARSET%" ^
        -type msdf -format png -size 48 -pxrange 4 -yorigin top -potr ^
        -imageout "%OUTPUT_DIR%%%~nF.png" -json "%OUTPUT_DIR%%%~nF.json"
    if errorlevel 1 (
        echo [Error] %%~nxF の生成に失敗しました
        set "RESULT=1"
    )
)

exit /b %RESULT%
