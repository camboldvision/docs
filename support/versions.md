# 版本与兼容性

更新日期：**2026 年 11 月 13 日**。文档对应当前 CamBold Viewer、CamBold SDK 和设备固件。

## 使用前核对

- 当前板端系统为 Debian 12 / AArch64。
- 当前 SDK 公共版本宏为 `CB_SDK_VERSION`，值为 `1.0.0`；开发时同步使用配套头文件与实现。
- 图像帧率、深度模式和功能可用性从设备读取；应用应支持设备返回的实际选项。
- 产品名称由设备上报，Viewer 可显示不同系列型号。

## 平台

| 项目 | 说明 |
| --- | --- |
| Windows Viewer | 当前操作与截图使用 Windows 版本 |
| USB SDK | 提供 Windows、Linux、macOS 构建入口；各系统需使用对应编译器和设备权限 |
| LOCAL SDK | 相机 Debian 12，公开 C API 与 USB 后端一致 |
| ROS 数据录制 | ROS1 rosbag；图像为 `sensor_msgs/CompressedImage` |
| 用户 USB 串口 | 独立的自定义通道，板端 `/dev/ttyGS2` |
| 文档站点 | Docsify 静态站点，本地资源，可部署到 GitHub Pages |

## 更新应用

升级 SDK 时，同步更新头文件和实现，再重新编译应用。升级固件后，重新查询能力与状态，并建立所需订阅。

