# 同帧颜色、视差与三维坐标

## 配置与开启

1. 连接后调用 `cb_request_stereo_status()`。
2. 在 `CB_EVENT_STEREO_STATUS` 中用 `cb_decode_stereo_status()` 读取状态与可用模式。
3. 确认标定有效、所需功能可用；停止 SLAM 并关闭深度后，设置模式、焦距与投影。
4. 等待配置执行成功，再开启深度。
5. 通过 `cb_get_disparity()` 获取同一帧的两个平面。

根据 `modes[]` 中的 `width`、`height`、`projection` 和 `max_fps` 选择模式，并使用该项的 `id`。

## 获取一帧数据

以下 C++ 片段在已连接并成功开启深度后调用；`camera` 为已有 `cb_t*`：

```cpp
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <vector>
#include "cb_sdk.h"

cb_disparity meta{};
const size_t capacity = CB_MAX_DISPARITY_WIDTH * CB_MAX_DISPARITY_HEIGHT;
std::vector<uint16_t> disparity(capacity), color(capacity);
uint64_t last_id = 0; // 在应用对象中保留；重连时重置为 0
int rc = cb_get_disparity(camera, last_id, &meta,
                          disparity.data(), disparity.size(),
                          color.data(), color.size());
if (rc == 0) {
    last_id = meta.frame_id;
    for (uint32_t v = 0; v < meta.height; ++v) {
        for (uint32_t u = 0; u < meta.width; ++u) {
            size_t i = size_t(v) * meta.width + u;
            float d = cb_fp16_to_float(disparity[i]);
            float xyz[3];
            if (!std::isfinite(d) || d <= 0 ||
                cb_disparity_to_point(&meta, float(u), float(v), d, xyz) != 0)
                continue;
            uint16_t c = color[i];
            uint8_t r = uint8_t(((c >> 11) & 31) * 255 / 31);
            uint8_t g = uint8_t(((c >> 5) & 63) * 255 / 63);
            uint8_t b = uint8_t((c & 31) * 255 / 31);
            // xyz：米；r/g/b：同帧同像素颜色。交给自己的点云或测距处理。
        }
    }
} else if (rc == -EAGAIN) {
    // 暂无比 last_id 更新的完整帧，稍后再取。
}
```

容量单位为 `uint16_t` 元素数。应用提供两个平面的缓冲，SDK 复制同一完整帧的视差和颜色。返回 `-ENOSPC` 时，按 `meta.width * meta.height` 扩容后重试。

## FP16 视差与 RGB565 颜色

`uint16_t` 数组保存 IEEE 754 binary16 位模式。调用 `cb_fp16_to_float()` 得到像素视差，再过滤非有限值和非正值。

颜色为 RGB565，三通道分别占 5 / 6 / 5 bit。`cb_get_disparity()` 返回本机字节序的 16 位值；事件解码接口返回小端字节平面，使用方式见 [深度 API](reference/api-depth.md)。

## 两种投影统一处理

`cb_disparity_to_point()` 按元数据返回校正左相机光学坐标：X 向右、Y 向下、Z 向前，单位为米。针孔和圆柱模式使用不同的几何关系，应用优先调用该接口。

![校正左相机的光学坐标方向](../assets/images/coordinates.svg)

`timestamp_monotonic_ns` 与对应源图像一致，用于跨流关联。深度 `frame_id` 和 UVC `sequence` 分别标识各自流中的帧。
