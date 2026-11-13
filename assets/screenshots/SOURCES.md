# 截图来源

更新日期：2026-11-13。

| 文件 | 来源与画面状态 |
| --- | --- |
| viewer-camera.png | 当前 Windows CamBold Viewer，连接实物相机并开启 UVC 预览 |
| viewer-depth.png | 同一 Viewer 与实物相机，水平 320 × 180 深度模式；校正左图、FP16 视差和点云为现场数据 |
| viewer-annotated.png | 在 viewer-depth.png 上叠加 1～6 控件编号 |
| viewer-recording.png | 真实 Viewer 的录制设置对话框，尚未开始录制 |
| viewer-terminal.png | 真实设备的 USB Linux 登录终端 |
| viewer-slam.png | Viewer 的 SLAM 页面，连接真实设备，处于待启动状态 |
| uvc-stereo.png | Wiki SDK 示例从当前设备保存的一帧 2160 × 1280 JPEG，等比缩小为 1296 × 768 PNG |

截图通过应用 WebView 捕获。系统指标为捕获时的读数，相机型号由设备上报。

更新截图时，先在 Viewer 中逐页捕获以上页面，再运行 `docs/tools/capture_wiki.py --source-dir <截图目录>`，复制图片并叠加编号。
