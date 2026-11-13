# 深度 API

异步控制与缓冲区约定见 [调用规则](reference/api.md)。

## cb_request_stereo_status

```c
int cb_request_stereo_status(cb_t *, uint64_t request_id);
```

请求深度功能可用性、当前参数和模式表。

**输入**：`host` 与 `request_id`。

**输出与返回值**：0 表示排队，随后通过 `CB_EVENT_STEREO_STATUS` 获取数据。

**说明**：先查状态，再从模式表选择输出尺寸、投影与帧率上限。

## cb_decode_stereo_status

```c
int cb_decode_stereo_status(const uint8_t *, size_t, cb_stereo_status *);
```

将深度状态事件解码为 `cb_stereo_status`。

**输入**：事件 `payload` 与长度；`out` 指向输出结构体。

**输出与返回值**：成功为 0；格式或参数无效为 `-EINVAL`。

**说明**：解码后，通过 `flags`、`error` 和 `message` 判断功能可用性。

## cb_set_disparity_enabled

```c
int cb_set_disparity_enabled(cb_t *, int enable, uint64_t request_id);
```

订阅或关闭深度输出。

**输入**：`host`；`enable` 为 0 关闭、非零开启；`request_id`。

**输出与返回值**：0 表示排队，最终状态看请求结果和深度状态。标定或资源缺失时设备会返回对应错误。

**说明**：开启时按模式上限调整并保存相机帧率，已有较低帧率保持不变。关闭后，如需提高帧率，调用 `cb_set_frame_rate()`。

## cb_set_disparity_config

```c
int cb_set_disparity_config(cb_t *, uint32_t mode_id, float focal_px, uint64_t request_id);
```

设置水平针孔模式的分辨率与像素焦距。

**输入**：`host`；从模式表选择的 `mode_id`；有限且大于 0 的 `focal_px`；`request_id`。

**输出与返回值**：0 表示排队，参数非法可返回 `-EINVAL`，设备仍会校验模式。

**说明**：先停止 SLAM 并关闭深度。该接口选择针孔投影，垂直圆柱模式使用扩展接口。

## cb_set_disparity_config_ex

```c
int cb_set_disparity_config_ex(cb_t *, uint32_t mode_id, float focal_px, uint32_t projection, float vertical_fov_deg, uint64_t request_id);
```

设置深度模式、焦距、投影与垂直 FOV。

**输入**：`host`、`mode_id`、有限正数 `focal_px`、`projection`（0 针孔 / 1 圆柱）、`vertical_fov_deg`（有限，30～180°）、`request_id`。

**输出与返回值**：0 表示排队，非法参数返回负错误码；执行结果异步返回。

**说明**：模式必须属于相应投影。先停止 SLAM 并关闭深度；参数持久保存，圆柱输出当前为 256 × 256。

## cb_get_disparity

```c
int cb_get_disparity(cb_t *, uint64_t after_id, cb_disparity *, uint16_t *disparity, size_t disparity_capacity, uint16_t *rgb565, size_t rgb565_capacity);
```

一次复制同帧视差、颜色与元数据。

**输入**：SDK 句柄；`after_id`（0 接受最新帧，其他值要求更新帧）；元数据输出；`disparity` 和 `rgb565` 两个 `uint16_t` 数组及各自元素容量。

**输出与返回值**：成功为 0。无更新完整帧 `-EAGAIN`。任一缓冲缺失或太小则 `-ENOSPC`，回填元数据供扩容，两个平面均不复制。

**说明**：两个容量均为元素数，输出为本机字节序的 16 位值，视差保存 FP16 位模式。接口线程安全，缓存仅保留最新完整帧；重连时将 `after_id` 重置为 0。

## cb_decode_disparity

```c
int cb_decode_disparity(const uint8_t *, size_t, cb_disparity *, const uint8_t **disparity_le, const uint8_t **rgb565_le);
```

直接解码 `CB_EVENT_DISPARITY` 中的配对帧。

**输入**：事件 payload 与长度；`out` 元数据；`disparity_le`、`rgb565_le` 为字节指针输出地址，可为 NULL 忽略该指针。

**输出与返回值**：成功为 0，返回两个小端 16 位平面的起始字节指针；无效帧 `-EINVAL`。

**说明**：指针借用 payload，回调返回即失效。按两个小端字节拼出 16 位值，再解码 FP16，以兼容未对齐数据。

## cb_fp16_to_float

```c
float cb_fp16_to_float(uint16_t bits);
```

将 IEEE 754 binary16 位模式转换为 C float。

**输入**：`bits`：一个 16 位浮点位模式。

**输出与返回值**：返回对应 float，包括零、非规格化数、无穷大和 NaN。

**说明**：函数通过软件转换 FP16 位模式。处理视差时过滤非有限值和非正值。

## cb_disparity_to_point

```c
int cb_disparity_to_point(const cb_disparity *, float u, float v, float disparity_px, float xyz[3]);
```

将一个像素及其视差转换为三维点。

**输入**：`cb_disparity` 元数据；像素 `u`、`v`；已解码的像素视差 `disparity_px`；`xyz[3]` 输出。

**输出与返回值**：成功为 0，XYZ 单位米；参数、视差或结果无效为 `-EINVAL`。

**说明**：按元数据自动处理针孔或圆柱投影。输出在校正左目光学坐标中，X 右、Y 下、Z 前；先确认 u/v 在帧尺寸内。
