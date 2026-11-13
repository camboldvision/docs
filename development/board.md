# 系统与交叉编译

CamBold 运行 **Debian 12 / AArch64**。用户可以安装应用、访问文件和网络、控制扩展接口，并使用板端 SDK 读取相机数据。

基础功能运行时，CPU 典型占用约 25%，GPU 资源空闲，NPU 保留一个核心供用户应用使用。资源余量随 SLAM 状态、深度分辨率和用户任务变化。

## 登录系统

通过 Viewer 的 `Terminal` 或网络 SSH 登录。第一次接触 Linux 请先阅读 [终端入门](guides/files-terminal.md)。

```bash
uname -m
cat /etc/os-release
df -h /data
free -h
```

设备报告的架构为 `aarch64`，系统为 Debian 12。用户应用可放在 `/data/share/my_app`，通过 MTP 管理。

## 推荐的交叉工具链

推荐版本：`gcc-arm-10.3-2021.07-aarch64-none-linux-gnu`。Windows 主机选择该版本的 Windows 工具包。

| 项目 | 地址 |
| --- | --- |
| 交叉工具链下载 | 下载地址待提供 |

交叉工具链在电脑上生成 ARM64 Linux 程序，供相机运行。

解压工具链后，在 PowerShell 验证路径，下面用 `D:/toolchains/aarch64-none-linux-gnu` 举例，应替换成自己的安装位置：

```powershell
& "D:/toolchains/aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc.exe" --version
```

## 交叉编译板端 SDK 示例

在项目根目录执行：

```powershell
cmake -S docs/wiki/examples/board -B build/wiki-board -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$PWD/docs/wiki/examples/board/toolchain.cmake" -DAARCH64_TOOLCHAIN_ROOT="D:/toolchains/aarch64-none-linux-gnu"
cmake --build build/wiki-board --parallel
```

该工程编译 SDK 的 `examples/camera.c`，使用 `LOCAL` 后端，生成 `cambold_board_reader`。附带的工具链文件可用于其他 CMake 工程。

## 部署到相机

通过 MTP 将 `cambold_board_reader` 复制到共享目录。也可以在电脑执行，IP 换成当前相机地址：

```powershell
scp build/wiki-board/cambold_board_reader cambold@192.168.1.100:/data/share/
```

随后在相机终端执行：

```bash
cd /data/share
chmod +x cambold_board_reader
./cambold_board_reader 10 1 1 0
```

运行应用时保持相机采集服务开启。LOCAL 通过本地通道获取数据，相机可独立运行。连接提示权限不足时，检查服务和本地接口权限，或用 `sudo` 运行示例以确认权限问题。

`Exec format error` 通常表示架构不对；`Permission denied` 应先检查执行权限；找不到本地服务时按 [服务维护](reference/files-services.md) 排查。

## 自己的程序与依赖

用户可以按需通过 `apt` 安装软件。先更新索引，再安装具体所需包，例如：

```bash
sudo apt update
sudo apt install python3
```

源码编译时选用与 Debian 12 AArch64 兼容的库和工具链。用户依赖放在独立目录，并为应用设置库查找路径，保留固件预装的驱动、运行库和服务程序。

Wi-Fi 6 可用于机器人网络和数据交换，Bluetooth 5.4 可用于兼容外设。两者使用标准 Linux 网络与蓝牙接口；连接 Wi-Fi 的步骤见 [网络设置](guides/files-terminal.md#连接-wi-fi)。
