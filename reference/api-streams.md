# 视频与 IMU API

回调、内存与线程约定见 [调用规则](reference/api.md)。

## cb_video_open

```c
int cb_video_open(const cb_video_config *config, cb_video_t **out_uvc);
```

创建独立的视频采集句柄。

**输入**：`config` 可为 NULL，或传 `cb_video_config` 指定回调和 `user`；`out_uvc` 为必填输出地址。

**输出与返回值**：成功为 0 并返回句柄；失败为负错误码。

**说明**：创建句柄后调用 `cb_video_start()` 开始采集，视频接口使用编译时选择的 USB / LOCAL 后端。

## cb_video_start

```c
int cb_video_start(cb_video_t *uvc, char *error, size_t error_capacity);
```

开始接收 JPEG 视频并调用帧回调。

**输入**：`uvc`：已创建的视频句柄；`error`、`error_capacity`：可选错误缓冲及字节容量，可用 NULL / 0 忽略。

**输出与返回值**：成功为 0；失败返回负值并尽可能写入 NUL 结尾错误文本。

**说明**：JPEG 属于 SDK，回调返回前复制需要保留的数据。USB 视频可能被其他软件独占。

## cb_video_stop

```c
void cb_video_stop(cb_video_t *uvc);
```

停止视频采集。

**输入**：`uvc`：视频句柄，NULL 可安全传入。

**输出与返回值**：无返回值。

**说明**：从应用线程调用。停止后保留句柄，可再次调用 `cb_video_start()`。

## cb_video_close

```c
void cb_video_close(cb_video_t *uvc);
```

停止视频并释放视频句柄。

**输入**：`uvc`：视频句柄，NULL 可安全传入。

**输出与返回值**：无返回值，之后不可使用该句柄。

**说明**：从应用线程调用，与 `cb_video_open()` 配对。

## cb_video_running

```c
int cb_video_running(const cb_video_t *uvc);
```

查询视频会话是否正在运行。

**输入**：`uvc`：视频句柄。

**输出与返回值**：运行中为 1，否则为 0；NULL 返回 0。

**说明**：返回视频会话状态。新帧通过视频回调接收。

## cb_decode_imu_batch

```c
int cb_decode_imu_batch(const uint8_t *payload, size_t payload_size, cb_imu_sample *out_samples, size_t *inout_sample_count);
```

解码一个 IMU 批次。

**输入**：`payload` 和字节长度；`out_samples` 为样本数组；`inout_sample_count` 输入数组可容纳的元素数。

**输出与返回值**：成功为 0，回填实际样本数；缓冲不足或数组为 NULL 返回 `-ENOSPC` 并回填需要数量；无效数据 `-EINVAL`。

**说明**：扩容和重新解码应在 payload 有效期间完成。加速度单位为 m/s²，角速度为 rad/s，时间为 ns。

## cb_decode_frame_meta

```c
int cb_decode_frame_meta(const uint8_t *payload, size_t payload_size, cb_frame_meta *out_frames, size_t *inout_frame_count);
```

解码一个图像元数据批次。

**输入**：事件 `payload` 和长度；`out_frames` 输出数组；`inout_frame_count` 输入元素容量。

**输出与返回值**：成功为 0 并回填数量；不足为 `-ENOSPC` 并回填所需数量；无效输入 `-EINVAL`。

**说明**：输出图像元数据；JPEG 通过视频回调获取，两者按源时间戳关联。
