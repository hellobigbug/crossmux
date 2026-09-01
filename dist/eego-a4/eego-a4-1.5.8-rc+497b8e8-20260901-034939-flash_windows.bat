@echo off
setlocal
REM EEGO A4 (ESP32-S3) 一键刷写脚本
REM 用法: 将本 bat 与三个 .bin 放同一目录, 双击运行
where esptool.py >nul 2>nul
if %errorlevel%==0 (
    set ESP=esptool.py
) else (
    set ESP=python -m esptool
)
set PORT=COM5
if not "%1"=="" set PORT=%1
echo 刷写端口: %PORT%
%ESP% --chip esp32s3 --port %PORT% --baud 460800 write_flash -z ^
  0x0000 eego-a4-1.5.8-rc+497b8e8-20260901-034939-bootloader.bin ^
  0x8000 eego-a4-1.5.8-rc+497b8e8-20260901-034939-partitions.bin ^
  0x10000 eego-a4-1.5.8-rc+497b8e8-20260901-034939-firmware.bin
echo 完成. 如失败请确认端口号, 用 "flash_windows.bat COMxx" 指定
pause
