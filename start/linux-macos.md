# Linux / macOS 连接

Linux / macOS 可通过 UVC 工具和 SDK 接入相机。文档中的 Viewer 截图和固件烧录操作使用 Windows 版本。

## Linux 电脑

连接相机后，在终端检查 USB 设备和视频设备：

```bash
lsusb
ls /dev/video*
```

`/dev/video0` 等编号会随电脑上的摄像头变化。使用系统摄像头应用选择 CamBold 视频设备即可预览。SDK 的 USB 控制和视频访问需要相应设备权限；出现 `Access denied` 时，先检查当前用户是否有 USB 和视频设备访问权限，并检查是否被其他程序占用。

可用管理员权限运行示例以确认权限问题；部署时按设备的实际 VID / PID 配置 udev 访问规则。

编译步骤见 [第一个 SDK 示例](development/host-sdk.md)。Linux AArch64 电脑作为外接主机时，应显式选择 `CB_SDK_BACKEND=USB`。

## macOS 电脑

使用支持外接 UVC 摄像头的应用选择 CamBold。SDK 编译时选择 `USB` 后端，并使用 CMake 与本机 C / C++ 编译器。若系统询问摄像头或 USB 配件访问权限，应允许相应应用访问。

macOS 可通过 MTP 工具访问相机的 `/data/share`，也可连接网络后使用 `scp` / `sftp` 传输文件。网络配置见 [Wi-Fi 与 SSH](guides/files-terminal.md#连接-wi-fi)。
