@echo off
echo Удаление ClearTemp...

echo Удаляю из автозагрузки...
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "ClearTemp" /f >nul 2>&1

echo Удаляю папку C:\Program Files\ClearTemp ...
rmdir /s /q "C:\Program Files\ClearTemp"

echo Готово. ClearTemp удалён.
pause
exit /b

