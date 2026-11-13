# 标定文件维护

深度与 SLAM 使用 `/data/eeprom/camera_imu_calibration.yaml`。该文件记录相机与 IMU 的标定参数；更换镜头、传感器或调整装配后，应重新标定。

## 查看与备份

```bash
ls -lh /data/eeprom
df -h /data/eeprom
cp /data/eeprom/camera_imu_calibration.yaml /data/share/camera_imu_calibration.backup.yaml
```

备份文件保存到 `/data/share` 后，可通过 MTP 复制到电脑。

## 替换步骤

1. 停止 SLAM，关闭深度，结束录制。
2. 备份现有标定。
3. 将新文件通过 MTP 放到 `/data/share/camera_imu_calibration.yaml`。
4. 确认文件来自当前设备、文件名与内容正确，并检查 EEPROM 可用空间。
5. 执行复制：

   ```bash
   sudo cp /data/share/camera_imu_calibration.yaml /data/eeprom/camera_imu_calibration.yaml
   sync
   ```

6. 重启相机，重新开启深度，检查标定状态和校正图像。
