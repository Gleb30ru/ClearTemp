@echo off
set size=%1
if "%size%"=="" set size=0.00

powershell -NoProfile -ExecutionPolicy Bypass -Command "Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.MessageBox]::Show('Было удалено %size% ГБ! Спасибо за то, что пользуетесь нами!', 'Temp.exe сработал!')"

exit /b

