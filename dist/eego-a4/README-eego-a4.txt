EEGO A4 烧录包 v1.5.8 (rc+497b8e8)  2026-09-01  中文版

设备: EEGO A4 (ESP32-S3, 原生USB)
固件特征: 诺基亚复古主题 + 电子宠物/今天吃什么/答案之书/星座/电影/每日一言/随机台词/流沙/赛博空调

三种刷写方式:
1) 双击 flash_windows.bat (确保已 pip install esptool, 默认 COM5)
2) 指定端口: flash_windows.bat COMx
3) 如用 PlatformIO 本机工程: pio run -e eego_a4 -t upload

esptool 地址表:
 bootloader  0x0000
 partitions  0x8000
 firmware    0x10000

firmware.elf 仅用于调试(GDB/崩溃回溯), 烧录不需要.
