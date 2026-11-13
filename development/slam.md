# SLAM 接入

应用通过 SDK 控制 SLAM，并接收位姿、跟踪图和地图数据。用户 ACM 串口用于自定义协议。

## 启动、停止与执行结果

连接相机后，SLAM 通道还需要完成准备。命令返回 `-ENOTCONN` 时可稍后重试；返回 0 表示命令已排队。应读取 JSON 中的 `state.last_result` 和运行状态，判断设备是否接受和完成操作。

```c
cb_slam_options options = {0};
options.map_name = "office_1";
options.map_mode = CB_SLAM_NEW;
options.dense_reconstruction = 0;
int rc = cb_slam_start(camera, &options);
/* 持续读取状态，确认启动；结束时 cb_slam_stop(camera)，等待保存完成。 */
```

使用 `CB_SLAM_RESUME` 继续建图，使用 `CB_SLAM_LOCALIZATION` 在已有地图中定位。地图名填写逻辑名称。程序启动时可能自动调整相机参数，运行期间保持所选模式。

## 200 Hz 位姿

```c
cb_slam_pose pose;
int rc = cb_slam_get_pose(camera, &pose);
if (rc == 0) {
    /* 与上次 timestamp_monotonic_ns 比较，判断是否为新样本。
       position 单位 m；velocity 单位 m/s；quaternion_xyzw 为 Hamilton 四元数。 */
}
```

该接口读取最新值缓存，初始化及地图对齐完成后，缓存按 200 Hz 更新。需要高频采集时，在独立线程中读取并比较时间戳；轮询间未读取的样本会被新值覆盖。界面可按独立频率绘制。

`run_id` 标识本次运行，`epoch` 标识位姿上下文。两者变化时，应用清理缓存或分段保存历史记录。位姿表示设备惯性参考系在地图坐标系中的位置与方向；计算相机位姿时，还需应用相机与设备参考系之间的标定变换。

## 图像与地图

`cb_slam_subscribe(camera, 1, 1)` 订阅跟踪预览和地图。使用 `cb_slam_poll_json()` 取出更新，用 `cb_slam_free_json()` 释放返回内存。

地图按关键帧 ID 增量发送。应用依次处理 reset、upsert、pose 和 erase，分别执行清空地图、新增或替换关键帧、更新位姿和删除关键帧。

每个 SDK 对象由一个消费者调用 `cb_slam_poll_json()` 取走待处理增量，其他线程通过 `cb_slam_get_json()` 读取状态快照。`cb_slam_clear_view()` 的作用范围限于本地显示缓存，运行控制和地图文件保持原状。

USB 稠密运行期间，开启 IMU 上传返回 `-EBUSY`；LOCAL 后端不受这一限制。多应用时，同一时刻由一个连接控制 SLAM 会话，其他连接可继续使用采集功能。完整接口见 [SLAM API](reference/api-slam.md)。
