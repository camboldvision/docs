# 连接与设备控制 API

异步结果、内存与线程约定见 [调用规则](reference/api.md)。

## cb_open

```c
int cb_open(const cb_config *config, cb_t **out_host);
```

创建 SDK 句柄，启动设备发现、连接和后台通信。

**输入**：`config`：可为 NULL 使用默认值，或传清零后的 `cb_config` 并设置事件回调和用户指针。`out_host`：必填句柄输出地址。

**输出与返回值**：成功返回 0 并写入句柄；失败返回负错误码。

**说明**：创建句柄后，等待连接事件或用 `cb_connected()` 确认连接。USB 自动发现设备，LOCAL 默认连接本地服务。

## cb_close

```c
void cb_close(cb_t *host);
```

关闭控制、调试和其他 SDK 后台工作，并释放句柄。

**输入**：`host`：由 `cb_open()` 创建的句柄；NULL 可安全传入。

**输出与返回值**：无返回值，调用后句柄不可再次使用。

**说明**：停止应用的读取线程后，从应用线程调用。视频句柄通过 `cb_video_close()` 单独关闭。

## cb_connected

```c
int cb_connected(const cb_t *host);
```

读取当前控制连接状态。

**输入**：`host`：SDK 句柄。

**输出与返回值**：已连接返回非零，未连接返回 0。

**说明**：立即返回连接状态。SLAM 的初始化状态通过 SLAM 状态接口读取。

## cb_refresh_connection

```c
int cb_refresh_connection(cb_t *host);
```

重新发出非阻塞的连接与能力探测。

**输入**：`host`：有效 SDK 句柄。

**输出与返回值**：成功返回 0，失败返回负错误码。

**说明**：用于应用先于设备或服务启动的场景，握手结果通过连接事件确认。

## cb_next_request_id

```c
uint64_t cb_next_request_id(cb_t *host);
```

为控制请求生成请求号。

**输入**：`host`：有效 SDK 句柄。

**输出与返回值**：返回 `uint64_t` 请求号。

**说明**：每条控制请求使用一个新请求号，通过 `CB_EVENT_RESULT.request_id` 关联执行结果。

## cb_get_control_port

```c
int cb_get_control_port(const cb_t *host, char *out, size_t capacity);
```

读取控制通道的逻辑名称。

**输入**：`host`、可写字符缓冲 `out` 和容量 `capacity`，容量单位为字节。

**输出与返回值**：成功返回字符串字节数，不含结尾 NUL。暂无通道为 `-ENOENT`，容量不足为 `-ENOSPC`，参数错误为 `-EINVAL`。

**说明**：USB 返回 `bulk-control`，LOCAL 返回 `local-control`。

## cb_get_debug_port

```c
int cb_get_debug_port(const cb_t *host, char *out, size_t capacity);
```

读取调试终端通道的逻辑名称。

**输入**：`host`、字符缓冲 `out` 和字节容量 `capacity`。

**输出与返回值**：成功返回不含 NUL 的字节数；暂无通道 `-ENOENT`，缓冲不足 `-ENOSPC`。

**说明**：调试通道打开后读取。USB 返回 `bulk-debug`，LOCAL 返回 `local-debug`。

## cb_get_device_name

```c
int cb_get_device_name(const cb_t *host, char *out, size_t capacity);
```

读取设备上报的产品名称，供应用显示。

**输入**：`host`、UTF-8 字符缓冲 `out` 和字节容量 `capacity`。

**输出与返回值**：成功返回名称字节数，不含 NUL；无名称 `-ENOENT`，容量不足 `-ENOSPC`，参数无效 `-EINVAL`。

**说明**：从缓存读取名称，连接时刷新，断线时清空。应用界面使用该名称显示产品型号。

## cb_request_metrics

```c
int cb_request_metrics(cb_t *host, uint64_t request_id);
```

向设备请求 CPU、内存和温度等运行状态。

**输入**：`host` 与 `request_id`。

**输出与返回值**：返回 0 表示排队成功；数据通过 `CB_EVENT_METRICS` 返回，并可用 `cb_get_metrics_json()` 读取缓存。

**说明**：按设备返回的字段解析状态，缺失字段按空值处理。

## cb_get_metrics_json

```c
int cb_get_metrics_json(const cb_t *host, char *out, size_t capacity);
```

复制最新系统状态 JSON。

**输入**：`host`、字符缓冲 `out` 和字节容量 `capacity`。

**输出与返回值**：成功返回 JSON 字节数，不含 NUL；还没有状态 `-ENOENT`，缓冲不足 `-ENOSPC`，参数错误 `-EINVAL`。

**说明**：读取最新缓存；调用 `cb_request_metrics()` 可请求刷新。

## cb_get_last_error

```c
int cb_get_last_error(const cb_t *host, char *out, size_t capacity);
```

复制 SDK 缓存的最近错误文本。

**输入**：`host`、字符缓冲 `out` 和字节容量 `capacity`。

**输出与返回值**：成功返回复制的字节数，不含 NUL；无错误时可能为空字符串；无效参数返回负值。

**说明**：文本按容量截断并补 NUL。单个控制请求的执行结果通过 `CB_EVENT_RESULT` 获取。

## cb_request_device_config

```c
int cb_request_device_config(cb_t *host, uint64_t request_id);
```

请求当前设备配置及可用图像帧率。

**输入**：`host` 与 `request_id`。

**输出与返回值**：返回 0 表示排队；收到 `CB_EVENT_DEVICE_CONFIG` 后解码。

**说明**：设备在连接握手后也会自动上报配置。界面按当前状态和 `frame_rates[]` 设置控件。

## cb_decode_device_config

```c
int cb_decode_device_config(const uint8_t *payload, size_t payload_size, cb_device_config *out_config);
```

将设备配置事件的数据解码为结构体。

**输入**：`payload`、`payload_size` 来自事件；`out_config` 为有效输出地址。

**输出与返回值**：成功为 0，填充 `cb_device_config`；格式或字段非法为 `-EINVAL`。

**说明**：按 `valid_flags` 判断帧率、IMU 和天线字段的有效性，帧率数组遍历范围为 `frame_rate_count` 项。

## cb_get_antenna

```c
int cb_get_antenna(cb_t *host, uint64_t request_id);
```

请求当前天线选择。

**输入**：`host` 与 `request_id`。

**输出与返回值**：排队成功为 0，结果由天线事件和请求结果返回；选择见 `event.antenna_wired`。

**说明**：0 为陶瓷天线，非零为外接天线。

## cb_set_antenna

```c
int cb_set_antenna(cb_t *host, int wired, uint64_t request_id);
```

设置天线选择。

**输入**：`host`；`wired` 为 0 选择陶瓷天线，非零选择外接天线；`request_id`。

**输出与返回值**：0 表示请求已排队，最终结果通过事件返回。

**说明**：天线配置会保存并对整个设备生效。切换外接天线前接好硬件。

## cb_set_imu_output

```c
int cb_set_imu_output(cb_t *host, int enable, uint64_t request_id);
```

开启或关闭本连接的 IMU 数据上传。

**输入**：`host`；`enable` 为 0 关闭、非零开启；`request_id`。

**输出与返回值**：排队成功为 0。USB 稠密 SLAM 运行期间请求开启返回 `-EBUSY`；其他失败返回负错误码。

**说明**：IMU 上传开机默认关闭，SLAM 内部的 IMU 独立运行。通过 `CB_EVENT_IMU_STATUS` 和请求结果确认开关状态。LOCAL 允许稠密重建与 IMU 上传同时运行。

## cb_set_frame_rate

```c
int cb_set_frame_rate(cb_t *host, uint32_t fps, uint64_t request_id);
```

请求修改全设备相机帧率。

**输入**：`host`；`fps` 为设备上报的合法档位；`request_id`。

**输出与返回值**：0 表示排队，设备最终校验结果由事件返回。

**说明**：深度或 SLAM 运行可能限制可选值。新帧率持久保存，并影响其他连接。
