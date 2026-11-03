# 包子 AI 聊天机器人

基于 XiaoZhi ESP32 2.5.2 的定制固件，适用于 **ESP32-S3 N16R8 + ST7789 240×240**，采用 `bread-compact-wifi-lcd` 默认接线。

## 当前功能

- 底部字幕单行左右往返滚动；能完整显示的短句居中静止。
- 滚动速度约 60 像素/秒，起始和两端停留约 1 秒。
- 待机隐藏完整顶部状态栏，唤醒后恢复；时钟和通知刷新不会使其重新出现。
- 保留原字体、表情和五个唤醒词：哈喽包子、你好包子、包子、包子包子、在吗包子。

## 文件

- [`source/`](source/)：可编辑源码、资源、依赖锁定文件和包子构建配置。
- [`新版完整固件`](firmware/baozi-five-wake-horizontal-idle-clean-full.bin)：包含上述全部修改。
- [`原始固件`](firmware/baozi-five-wake-original-full.bin)：最初提供的 2.0.2 固件，供恢复参考。
- [`源码压缩包`](firmware/baozi-horizontal-source.zip)：完整修改源码的便携副本。
- [`烧录说明`](firmware/烧录说明.md)、[`校验报告`](firmware/firmware-verification.json)、[`补丁`](firmware/subtitle-scroll.patch)。
- [`verification/`](verification/)：主机测试、显示控件测试和已去除设备联网标识的构建/启动记录。

## 编译

安装 ESP-IDF **v6.1**，在其环境中将仓库放到纯英文路径：

```sh
cd source
python scripts/build.py bread-compact-wifi-lcd -c baozi.json --name bread-compact-wifi-lcd-baozi --language zh-CN
```

输出 `source/build/merged-binary.bin`。首次构建会自动下载依赖；中国大陆可设置 `IDF_COMPONENT_STORAGE_URL=https://components-file.espressif.cn`。

## 烧录与验证

完整 BIN 烧录到 **0x0**，不是单应用 OTA 文件。完整烧录覆盖配置区域，可能需要重新配网与激活。

已读取硬件信息，确认 16 MB Flash、8 MB PSRAM。110 项主机测试、LVGL 字幕/待机状态栏测试、编译链接与二进制校验通过。新固件尚未由本项目流程烧录到实机验证。

SDK、工具链及编译缓存通过官方安装步骤重新获取，不纳入版本控制。上游源码许可证见 [`LICENSE`](LICENSE)，第三方资源保留各自原有许可信息。
