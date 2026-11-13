# SLAM API

执行结果与内存约定见 [调用规则](reference/api.md)。

## cb_slam_start

```c
int cb_slam_start(cb_t *, const cb_slam_options *);
```

启动新建图、继续建图或纯定位。

**输入**：`host` 与 `cb_slam_options`，含地图名、运行模式和稠密开关。

**输出与返回值**：0 表示命令已排队；握手未就绪 `-ENOTCONN`，无效参数等返回负错误码。

**说明**：通过 JSON 的 `state.last_result` 和状态确认启动。加载地图可能自动修改相机参数，地图模式在运行中固定。USB 稠密运行关闭本连接 IMU 上传。

## cb_slam_stop

```c
int cb_slam_stop(cb_t *);
```

请求停止 SLAM，建图模式下保存地图。

**输入**：`host`。

**输出与返回值**：0 表示排队，失败为负错误码。

**说明**：等待设备状态确认停止和保存完成，再复制地图、断电或修改几何参数。纯定位不修改原地图。

## cb_slam_refresh_maps

```c
int cb_slam_refresh_maps(cb_t *);
```

刷新设备地图目录。

**输入**：`host`。

**输出与返回值**：0 表示排队；更新后可从 JSON 的 `map_catalog` 读取列表。

**说明**：通过 MTP 新增或删除地图后，再次调用以更新设备地图列表。

## cb_slam_subscribe

```c
int cb_slam_subscribe(cb_t *, int preview, int map);
```

控制跟踪预览和地图数据的订阅。

**输入**：`host`；`preview` 非零订阅预览；`map` 非零订阅地图。

**输出与返回值**：0 表示排队，通道未就绪 `-ENOTCONN` 等。

**说明**：该接口控制显示数据的接收，SLAM 按原状态运行。停止算法使用 `cb_slam_stop()`。

## cb_slam_imu_upload_blocked

```c
int cb_slam_imu_upload_blocked(cb_t *);
```

查询当前 SDK 后端是否限制 IMU 上传。

**输入**：`host`。

**输出与返回值**：有效连接上，USB 稠密运行返回非零，允许上传为 0；LOCAL 始终为 0；调用失败可能返回负错误码。

**说明**：先检查负错误码，再判断限制状态。停止稠密运行后解除限制，应用可重新调用 `cb_set_imu_output()` 开启上传。

## cb_slam_get_pose

```c
int cb_slam_get_pose(cb_t *, cb_slam_pose *);
```

读取最新有效 SLAM 位姿。

**输入**：`host`；`cb_slam_pose*` 输出地址。

**输出与返回值**：成功 0；初始化、地图对齐前或断线时没有有效数据为 `-EAGAIN`；参数错误为负值。

**说明**：初始化后缓存按 200 Hz 更新，读取保留缓存内容，通过比较时间戳识别新样本。轮询间未读取的样本会被新值覆盖。

## cb_slam_get_json

```c
int cb_slam_get_json(cb_t *, char **out_json);
```

读取 SLAM 状态快照，不消费待处理增量。

**输入**：`host`；`out_json` 为 `char*` 的输出地址。

**输出与返回值**：成功 0 并分配 UTF-8、NUL 结尾 JSON；失败为负值。包含状态、位姿、显示字段描述和地图目录。

**说明**：用 `cb_slam_free_json()` 释放。大整数标识和时间戳可使用十进制字符串表示，解析时保留精度。图像和地图增量通过 `cb_slam_poll_json()` 获取。

## cb_slam_poll_json

```c
int cb_slam_poll_json(cb_t *, char **out_json);
```

获取状态，并取走待处理图像与地图增量。

**输入**：`host`；`out_json` 为字符串输出地址。

**输出与返回值**：成功 0，分配 JSON；有更新时包含图像及 `map_delta`，失败为负值。

**说明**：每个 SDK 对象只允许一个增量消费者。逐次应用 reset / upsert / pose / erase，处理后用专用函数释放字符串。

## cb_slam_free_json

```c
void cb_slam_free_json(char *);
```

释放由 SLAM JSON 接口分配的字符串。

**输入**：`json`：来自 `cb_slam_get_json()` 或 `cb_slam_poll_json()` 的指针；NULL 可安全传入。

**输出与返回值**：无返回值。

**说明**：每个返回字符串由此函数释放一次。

## cb_slam_clear_view

```c
int cb_slam_clear_view(cb_t *);
```

清空本机 SDK 的显示缓存。

**输入**：`host`。

**输出与返回值**：成功 0，失败为负错误码。

**说明**：清除范围限于本地显示缓存。重新订阅并同步状态可恢复完整显示。
