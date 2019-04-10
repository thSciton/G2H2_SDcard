set rev=0
for /F "eol=; tokens=4" %%i in ('findstr build_ver: src\components\Style.qml') do set rev=%%i
echo %rev%
set rev=%rev:"=%
echo %rev% > UI_rev
