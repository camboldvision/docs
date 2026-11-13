# 终端与板端录制 API

异步控制与句柄约定见 [调用规则](reference/api.md)。

## cb_debug_open

```c
int cb_debug_open(cb_t *host, const char *port, uint32_t baudrate);
```

打开 Linux 调试终端通道。

**输入**：`host`；`port` 可为 NULL，USB 可使用 `gser.0` / `bulk0`；`baudrate` 保留但被忽略。

**输出与返回值**：成功 0，失败为负错误码。

**说明**：USB 使用独立调试接口，LOCAL 以调用者权限创建私有 shell 终端。用户 ACM 保持独立。

## cb_debug_close

```c
void cb_debug_close(cb_t *host);
```

关闭本句柄的调试终端和相关工作。

**输入**：`host`。

**输出与返回值**：无返回值。

**说明**：关闭后停止向该通道写入，主控制连接保持开启。

## cb_debug_write

```c
int cb_debug_write(cb_t *host, const uint8_t *data, size_t size);
```

将字节加入终端发送队列。

**输入**：`host`、`data`、字节数 `size`；size 为 0 时可传 NULL。

**输出与返回值**：排队成功返回 0；未打开为 `-ENODEV`，队列满为 `-EAGAIN`，无效参数返回负错误码。

**说明**：命令末尾发送换行，通过终端回显确认执行结果。

## cb_debug_read

```c
int cb_debug_read(cb_t *host, uint8_t *data, size_t size, int timeout_ms);
```

读取终端接收字节。

**输入**：`host`、可写缓冲 `data`、字节容量 `size`；`timeout_ms` 为 0 轮询，正值最多等指定毫秒，负值一直等到数据或关闭。

**输出与返回值**：返回实际字节数；超时或已关闭为 0；出错为负值。

**说明**：按返回长度处理原始字节，其中可能包含 UTF-8 文本和终端控制序列。需要 C 字符串时，由应用预留空间并补 NUL。

## cb_set_terminal_size

```c
int cb_set_terminal_size(cb_t *host, uint16_t cols, uint16_t rows, uint64_t request_id);
```

更新终端的字符列数和行数。

**输入**：`host`；`cols`、`rows` 为有效字符尺寸；`request_id`。

**输出与返回值**：0 表示请求排队，执行结果异步返回。

**说明**：窗口尺寸变化后调用。USB 更新设备调试终端，LOCAL 更新本对象的私有终端，尺寸单位为字符。

## cb_rosbag_start

```c
int cb_rosbag_start(cb_t *host, const char *name, int record_image, int record_imu, uint64_t request_id);
```

请求在相机上录制 rosbag。

**输入**：`host`；`name` 为非空文件名；`record_image` 控制图像；`record_imu` 当前必须为非零；`request_id`。

**输出与返回值**：0 表示排队；`record_imu=0` 返回 `-ENOTSUP`，名称非法或过长返回负错误码；状态经事件返回。

**说明**：文件保存到板端 `/data/share`，名称参数仅保留文件名部分。Viewer 的电脑端录制操作见 [录制 ROS1 rosbag](guides/recording.md)。

## cb_rosbag_stop

```c
int cb_rosbag_stop(cb_t *host, uint64_t request_id);
```

请求停止板端 rosbag 录制并关闭文件。

**输入**：`host` 与 `request_id`。

**输出与返回值**：0 表示排队，最终结果和录制状态由事件返回。

**说明**：等待板端录制停止、文件关闭后，再备份或断电。
