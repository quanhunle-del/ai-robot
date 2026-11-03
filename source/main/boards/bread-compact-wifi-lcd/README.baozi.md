# Baozi 240x240 build

Hardware: ESP32-S3 N16R8, default bread-compact-wifi-lcd wiring, ST7789 240x240.

Build using ESP-IDF v6.1:

```sh
python scripts/build.py bread-compact-wifi-lcd -c baozi.json --name bread-compact-wifi-lcd-baozi --language zh-CN
```

The variant retains the original 16 MiB partition offsets and the verified assets
from baozi-five-wake-full.bin (five wake phrases, font and expressions).
Default-style subtitles use one horizontal back-and-forth scrolling line; text
that fits remains centered. CR/LF are displayed as spaces in single-line mode.

The output is a merged image for address 0x0, not an OTA application image.

The complete top status bar is hidden while idle and restored outside idle.
Parent-layer hiding also covers periodic clock updates and notification timers.
