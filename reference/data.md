# 数据结构、事件和单位

结构体的字段顺序和类型以 `cb_sdk.h` 为准。使用 C 的 `{0}` 或 C++ 的 `{}` 初始化；跨语言封装保持 C ABI 的字段布局与对齐方式。

## 公共配置与回调

| 结构 / 字段 | 类型或含义 |
| --- | --- |
| `cb_config.control_port` | 可选字符串；USB 默认自动发现，LOCAL 默认本地服务，可显式指定本地 socket 路径 |
| `control_baudrate` | 保留项，USB bulk 和 LOCAL 忽略 |
| `callback` | `cb_event_callback`，签名为 `void(const cb_event*, void*)` |
| `user` | 应用指针，原样传入回调 |
| `cb_video_config.callback` | `cb_video_frame_callback`，签名为 `void(const cb_video_frame*, void*)` |
| `cb_video_config.user` | 视频回调的应用指针 |

`cb_t` 和 `cb_video_t` 是不透明句柄，应用通过公开 API 操作。

## cb_event

| 字段 | 含义 |
| --- | --- |
| `type` | 事件类型，见下表 |
| `request_id` | 与控制请求对应的请求号 |
| `result` | 请求结果，0 成功、负值错误；按事件类型读取 |
| `antenna_wired` | 天线状态：0 陶瓷、非零外接 |
| `enabled` | 对应功能的开关状态 |
| `payload` / `payload_size` | 借用的二进制数据及字节长度 |
| `text` | 可选文本；判空后使用 |

按事件类型读取对应字段。payload 和 text 在回调期间有效，需要保留时复制到应用缓冲。

| 事件常量 | 用途 / 处理方式 |
| --- | --- |
| `CB_EVENT_CONNECTED` | 连接已建立，重新读取能力与所需状态 |
| `CB_EVENT_DISCONNECTED` | 断线，清理本轮缓存 |
| `CB_EVENT_RESULT` | 控制请求执行结果，通过 request_id 关联 |
| `CB_EVENT_METRICS` | 系统指标 JSON，可读取指标缓存 |
| `CB_EVENT_ANTENNA` | 天线状态 |
| `CB_EVENT_ROSBAG` | 板端录制状态 |
| `CB_EVENT_IMU_STATUS` | IMU 上传状态 |
| `CB_EVENT_STREAM_STATUS` | 采集流状态 |
| `CB_EVENT_ERROR` | 错误信息 |
| `CB_EVENT_FRAME_META` | 用 `cb_decode_frame_meta()` 解码 |
| `CB_EVENT_IMU_BATCH` | 用 `cb_decode_imu_batch()` 解码 |
| `CB_EVENT_SENSOR_STATUS` | 传感器状态通知，按设备消息展示 |
| `CB_EVENT_DEVICE_CONFIG` | 用 `cb_decode_device_config()` 解码 |
| `CB_EVENT_STEREO_STATUS` | 用 `cb_decode_stereo_status()` 解码 |
| `CB_EVENT_DISPARITY` | 用 `cb_decode_disparity()` 解码，或在应用线程调用配对 getter |

## cb_device_config

| 字段 | 含义 |
| --- | --- |
| `valid_flags` | `CB_CONFIG_FRAME_RATE_VALID`、`CB_CONFIG_IMU_VALID`、`CB_CONFIG_ANTENNA_VALID` 位或组合 |
| `frame_rate` | 当前图像 fps |
| `imu_output_enabled` | IMU 上传 0 / 1 |
| `antenna_wired` | 天线选择 0 / 1 |
| `frame_rate_count` | 可用帧率数量 |
| `frame_rates[CB_MAX_FRAME_RATES]` | 可用档位；容量 64，仅前 count 项有效 |

## cb_video_frame

| 字段 | 含义 / 单位 |
| --- | --- |
| `jpeg` / `jpeg_size` | 原始双目 MJPEG 帧及字节长度 |
| `sequence` | 视频流中的帧序号 |
| `timestamp_monotonic_ns` | 源左图曝光中点，板端单调时钟 ns |
| `width` / `height` | 这张 JPEG 的实际宽高，当前原始双目为 2160 × 1280 |
| `flags` | 时间戳、左右曝光有效性位 |
| `exposure_us[2]` | 左 / 右曝光时长，单位微秒 |

使用时间戳和左右目曝光字段前，分别检查 `CB_VIDEO_TIMESTAMP_VALID`、`CB_VIDEO_EXPOSURE_LEFT_VALID` 和 `CB_VIDEO_EXPOSURE_RIGHT_VALID`。

## cb_imu_sample 与 cb_frame_meta

| 结构 / 字段 | 含义 |
| --- | --- |
| IMU `sequence` | 样本序号 |
| IMU `timestamp_monotonic_ns` | 板端单调时钟 ns |
| IMU `accel[3]` | 传感器三轴加速度，m/s² |
| IMU `gyro[3]` | 三轴角速度，rad/s |
| IMU `temperature` | 保留字段，当前无有效温度值 |
| IMU `flags` | 样本状态位，按固件定义使用 |
| 帧元数据 `sequence` / `timestamp_monotonic_ns` | 帧序号与源时间 |
| `payload_size` | 图像负载大小，字节 |
| `width` / `height` / `stride` | 图像宽高和元数据中的步长 |
| `format` / `flags` | 格式标识与状态；元数据本身不含像素 |

IMU 向量采用传感器坐标系。转换到相机光学坐标系时，应用对应的标定变换。

## cb_stereo_mode 与 cb_stereo_status

| 字段 | 含义 |
| --- | --- |
| mode `id` | 模式 ID |
| mode `width` / `height` | 用户输出平面的尺寸 |
| mode `network_width` / `network_height` | 兼容字段；输出图像尺寸读取 `width` / `height` |
| mode `projection` | 0：`CB_PERSPECTIVE`；1：`CB_CYLINDRICAL` |
| mode `max_fps` | 模式允许的最高图像 / 深度 fps |
| status `flags` | 可用性与开关位 |
| status `error` / `message` | 当前错误码和可读原因 |
| status `mode_id` / `focal_px` | 当前模式和水平像素焦距 |
| status `mode_count` / `modes[16]` | 设备上报模式表，只访问 count 项 |
| status `projection` / `vertical_fov_deg` | 当前投影与垂直视场角（度） |

状态标志包括 `CB_STEREO_AVAILABLE`、`CB_STEREO_CALIBRATION_VALID`、`CB_STEREO_MODELS_VALID` 和 `CB_STEREO_ENABLED`，可同时置位。应用按所需标志逐项检查。

## cb_disparity

| 字段 | 含义 |
| --- | --- |
| `generation` | 数据配置代次，几何变化时应分段处理缓存 |
| `frame_id` | 深度流帧标识 |
| `timestamp_monotonic_ns` | 对应源图像的曝光中点，ns |
| `mode_id`、`width`、`height` | 模式与两平面共同尺寸 |
| `focal_px` | 水平焦距，像素 |
| `cx` / `cy` | 输出图像主点，像素 |
| `baseline_metres` | 基线，米 |
| `disparity_format` | `CB_DISPARITY_FP16` |
| `projection` | 针孔或圆柱 |
| `fy` | 针孔时为垂直像素焦距；圆柱时为像素 / 弧度 |
| `vertical_fov_deg` | 垂直视场角，度 |

每个平面有 width × height 个 16 位元素。`CB_MAX_DISPARITY_WIDTH` / `HEIGHT` 定义的缓冲上界为 512 × 512，可选输出尺寸由设备模式表提供。

针孔投影中，Z = fx × B / d；圆柱投影中，fx × B / d 表示圆柱半径。`cb_disparity_to_point()` 按投影参数计算三维坐标。

## cb_slam_options 与 cb_slam_pose

| 字段 | 含义 |
| --- | --- |
| options `map_name` | 地图逻辑名称，字符串 |
| options `map_mode` | `CB_SLAM_NEW` / `CB_SLAM_RESUME` / `CB_SLAM_LOCALIZATION` |
| options `dense_reconstruction` | 0 稀疏，非零稠密 |
| pose `run_id` | 本次运行标识 |
| pose `timestamp_monotonic_ns` | 位姿源时刻，ns |
| pose `frame_id` | 位姿关联帧标识；跨数据流同步使用源时间戳 |
| pose `epoch` | 位姿上下文代次 |
| pose `position[3]` | 设备惯性参考系在地图中的位置，m |
| pose `quaternion_xyzw[4]` | Hamilton 四元数，顺序 x、y、z、w |
| pose `velocity[3]` | 地图坐标速度，m/s |

深度 XYZ 位于校正左相机光学坐标系。转换到地图坐标时，先应用相机到设备惯性参考系的标定变换，再应用对应时刻的 SLAM 位姿。

## SLAM JSON 与增量地图

状态快照提供 `state`、`pose`、`descriptors` 和 `map_catalog`，其中 `descriptors` 为界面显示字段说明。`poll_json()` 额外取走图像与地图更新。解析器按实际返回内容处理可选字段。

`map_delta` 包括 reset 与按关键帧 ID 的 upsert / pose / erase。点数据 `xyzrgb16` 为 base64 编码，每点 16 字节：小端 float32 X/Y/Z、RGB888 三字节、一个填充字节。点位于各关键帧的设备惯性参考系，应用对应关键帧位姿后得到地图坐标。稀疏和稠密使用同一显示通道。

标识和纳秒时间可使用十进制字符串表示。JavaScript 使用字符串或 BigInt 保留 64 位整数精度。
