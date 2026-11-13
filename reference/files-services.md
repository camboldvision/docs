# 文件路径与服务维护

## 常用路径

| 路径 | 内容 | 用户建议 |
| --- | --- | --- |
| `/data/share` | MTP 共享目录、默认地图和板端录制文件 | 存放用户应用和备份 |
| `/data/eeprom` | 16 KiB EEPROM 文件系统 | 仅放标定和小型设备配置 |
| `/data/eeprom/camera_imu_calibration.yaml` | 当前设备标定 | 替换前备份，并确保与实物一致 |
| `/data/eeprom/camera_config.json` | 持久相机配置 | 优先用 Viewer / SDK 修改 |
| `/usr/bin/cambold_slam` | 预装建图定位程序 | 随固件提供 |
| `/usr/share/cambold_slam/config` | 预装功能配置 | 通过 Viewer 控制功能 |
| `/run` | 临时运行文件 | 仅用于本次运行，持久数据放在 `/data/share` |

## 查看服务

Linux 使用 systemd 管理开机服务。`active (running)` 表示服务进程正在运行，SLAM 的运行状态在 Viewer 中查看。

```bash
systemctl status eeprom-utfs.service --no-pager
systemctl status stereo-imu-capture.service --no-pager
systemctl status cambold_slam.service --no-pager
```

不同固件的采集服务名称可能不同，可用下面的命令查找已安装单元：

```bash
systemctl list-unit-files | grep -E 'camera|stereo|cambold|eeprom'
```

## 获取故障记录

```bash
journalctl -b -u cambold_slam.service --no-pager -n 100
journalctl -b -p warning --no-pager -n 100
df -h /data
```

`-b` 表示本次开机，`-n 100` 表示最近 100 行。若固件已保留上次启动日志，可用 `journalctl -b -1` 查看前一次记录。

反馈问题时，提供固件版本、设备名称、运行功能、错误原文和相关日志。故障发生后，在重启前保存记录。停止或重启服务前，先结束录制和建图并等待文件保存完成。
