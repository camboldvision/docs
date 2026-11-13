// User example: USB discovery, asynchronous requests, image, IMU and disparity.
#include "cb_sdk.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

struct State {
  std::mutex mutex;
  std::condition_variable changed;
  std::map<uint64_t, int> results;
  cb_device_config config{};
  cb_stereo_status stereo{};
  bool have_config = false, have_stereo = false, collect_imu = false;
  std::vector<cb_imu_sample> imu;
};
static void on_event(const cb_event *event, void *user) {
  auto &s = *static_cast<State *>(user);
  std::lock_guard<std::mutex> guard(s.mutex);
  if (event->type == CB_EVENT_RESULT)
    s.results[event->request_id] = event->result;
  else if (event->type == CB_EVENT_DEVICE_CONFIG)
    s.have_config = cb_decode_device_config(event->payload, event->payload_size, &s.config) == 0;
  else if (event->type == CB_EVENT_STEREO_STATUS)
    s.have_stereo = cb_decode_stereo_status(event->payload, event->payload_size, &s.stereo) == 0;
  else if (event->type == CB_EVENT_IMU_BATCH && s.collect_imu) {
    cb_imu_sample samples[64];
    size_t count = 64;
    if (!cb_decode_imu_batch(event->payload, event->payload_size, samples, &count) &&
        s.imu.size() + count <= 16000)
      s.imu.insert(s.imu.end(), samples, samples + count);
  }
  s.changed.notify_all();
}
class Device {
public:
  State state;
  std::unique_ptr<cb_t, decltype(&cb_close)> host{nullptr, cb_close};
  bool stop_stereo = false, stop_imu = false;
  Device() {
    cb_config config{};
    config.callback = on_event;
    config.user = &state;
    cb_t *raw = nullptr;
    if (cb_open(&config, &raw)) throw std::runtime_error("cb_open failed");
    host.reset(raw);
    const auto deadline = std::chrono::steady_clock::now() + 15s;
    while (!cb_connected(host.get()) && std::chrono::steady_clock::now() < deadline)
      std::this_thread::sleep_for(100ms);
    if (!cb_connected(host.get())) throw std::runtime_error("Camera connection timed out");
    cb_request_device_config(host.get(), cb_next_request_id(host.get()));
    cb_request_stereo_status(host.get(), cb_next_request_id(host.get()));
    std::unique_lock<std::mutex> lock(state.mutex);
    if (!state.changed.wait_for(lock, 10s, [&] { return state.have_config && state.have_stereo; }))
      throw std::runtime_error("Device configuration timed out");
  }
  ~Device() {
    // Restore runtime output switches opened by this example.
    try {
      if (stop_stereo) request([&](uint64_t id) { return cb_set_disparity_enabled(host.get(), 0, id); });
      if (stop_imu) request([&](uint64_t id) { return cb_set_imu_output(host.get(), 0, id); });
    } catch (const std::exception &e) { std::cerr << "Cleanup: " << e.what() << '\n'; }
  }
  template <typename Send> void request(Send send) {
    const auto id = cb_next_request_id(host.get());
    const int queued = send(id);
    if (queued) throw std::runtime_error("Request submission failed: " + std::to_string(queued));
    std::unique_lock<std::mutex> lock(state.mutex);
    if (!state.changed.wait_for(lock, 15s, [&] { return state.results.count(id) != 0; }))
      throw std::runtime_error("Request completion timed out");
    const int result = state.results.at(id);
    state.results.erase(id);
    if (result) throw std::runtime_error("Device rejected request: " + std::to_string(result));
  }
};

struct JpegCapture {
  std::mutex mutex;
  std::condition_variable ready;
  int skip = 15;
  uint64_t sequence = 0, timestamp = 0;
  uint32_t exposure[2]{}, width = 0, height = 0;
  std::vector<uint8_t> bytes;
};
static void on_jpeg(const cb_video_frame *frame, void *user) {
  auto &s = *static_cast<JpegCapture *>(user);
  std::lock_guard<std::mutex> lock(s.mutex);
  if (s.skip) { --s.skip; return; }
  if (!s.bytes.empty() || !(frame->flags & CB_VIDEO_TIMESTAMP_VALID)) return;
  // The SDK owns frame->jpeg. Copy before returning from the callback.
  s.bytes.assign(frame->jpeg, frame->jpeg + frame->jpeg_size);
  s.sequence = frame->sequence; s.timestamp = frame->timestamp_monotonic_ns;
  s.exposure[0] = frame->exposure_us[0]; s.exposure[1] = frame->exposure_us[1];
  s.width = frame->width; s.height = frame->height;
  s.ready.notify_all();
}
static void save_jpeg(const std::string &path) {
  JpegCapture data;
  cb_video_config config{}; config.callback = on_jpeg; config.user = &data;
  cb_video_t *raw = nullptr;
  if (cb_video_open(&config, &raw)) throw std::runtime_error("UVC open failed");
  std::unique_ptr<cb_video_t, decltype(&cb_video_close)> uvc(raw, cb_video_close);
  char error[512]{};
  if (cb_video_start(uvc.get(), error, sizeof(error))) throw std::runtime_error(error);
  std::unique_lock<std::mutex> lock(data.mutex);
  if (!data.ready.wait_for(lock, 20s, [&] { return !data.bytes.empty(); }))
    throw std::runtime_error("UVC frame timed out");
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char *>(data.bytes.data()), data.bytes.size());
  if (!file) throw std::runtime_error("Cannot write JPEG file");
  std::cout << "JPEG " << data.width << 'x' << data.height << " sequence=" << data.sequence
            << " timestamp_ns=" << data.timestamp << " exposure_us=" << data.exposure[0]
            << ',' << data.exposure[1] << " file=" << path << '\n';
}
static void read_imu(Device &device, int seconds) {
  bool was_enabled;
  {
    std::lock_guard<std::mutex> lock(device.state.mutex);
    was_enabled = device.state.config.imu_output_enabled != 0;
  }
  device.stop_imu = !was_enabled;
  device.request([&](uint64_t id) { return cb_set_imu_output(device.host.get(), 1, id); });
  // Allow the newly enabled stream to drain its initial buffered records.
  std::this_thread::sleep_for(500ms);
  {
    std::lock_guard<std::mutex> lock(device.state.mutex);
    device.state.imu.clear();
    device.state.collect_imu = true;
  }
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  std::lock_guard<std::mutex> lock(device.state.mutex);
  device.state.collect_imu = false;
  if (device.state.imu.empty()) throw std::runtime_error("No IMU samples received");
  std::cout << "IMU samples=" << device.state.imu.size() << '\n';
  for (size_t i = 0; i < std::min<size_t>(10, device.state.imu.size()); ++i) {
    const auto &s = device.state.imu[i];
    std::cout << s.sequence << " timestamp_ns=" << s.timestamp_monotonic_ns
              << " accel_m_s2=" << s.accel[0] << ',' << s.accel[1] << ',' << s.accel[2]
              << " gyro_rad_s=" << s.gyro[0] << ',' << s.gyro[1] << ',' << s.gyro[2] << '\n';
  }
}
static void save_depth(Device &device, const std::string &prefix) {
  {
    std::lock_guard<std::mutex> lock(device.state.mutex);
    if (!(device.state.stereo.flags & CB_STEREO_AVAILABLE))
      throw std::runtime_error(device.state.stereo.message);
    device.stop_stereo = !(device.state.stereo.flags & CB_STEREO_ENABLED);
  }
  device.request([&](uint64_t id) { return cb_set_disparity_enabled(device.host.get(), 1, id); });
  cb_disparity frame{};
  std::vector<uint16_t> disparity, rgb;
  const auto deadline = std::chrono::steady_clock::now() + 25s;
  int result = -EAGAIN;
  while (std::chrono::steady_clock::now() < deadline) {
    result = cb_get_disparity(device.host.get(), 0, &frame, disparity.data(), disparity.size(), rgb.data(), rgb.size());
    if (!result) break;
    if (result == -ENOSPC) {
      disparity.resize(size_t(frame.width) * frame.height); rgb.resize(disparity.size());
    } else if (result != -EAGAIN) throw std::runtime_error("Disparity read failed: " + std::to_string(result));
    std::this_thread::sleep_for(20ms);
  }
  if (result) throw std::runtime_error("Disparity frame timed out");
  std::ofstream f16(prefix + ".f16", std::ios::binary), ppm(prefix + ".ppm", std::ios::binary);
  // Raw IEEE-754 binary16 samples in little-endian row-major file order.
  // Width, height, format and reconstruction parameters are in the JSON sidecar.
  ppm << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
  auto color = [](uint16_t c, int channel) -> uint8_t {
    return channel == 0 ? ((c >> 11) & 31) * 255 / 31 :
           channel == 1 ? ((c >> 5) & 63) * 255 / 63 : (c & 31) * 255 / 31;
  };
  size_t count = 0;
  for (size_t i = 0; i < disparity.size(); ++i) {
    f16.put(char(disparity[i] & 255)); f16.put(char(disparity[i] >> 8));
    for (int c = 0; c < 3; ++c) ppm.put(char(color(rgb[i], c)));
    const double d = cb_fp16_to_float(disparity[i]);
    float xyz[3];
    if (!cb_disparity_to_point(&frame,float(i%frame.width),float(i/frame.width),float(d),xyz)) {
      const double distance=std::sqrt(double(xyz[0])*xyz[0]+double(xyz[1])*xyz[1]+double(xyz[2])*xyz[2]);
      if (distance >= 0.03 && distance <= 10) ++count;
    }
  }
  std::ofstream ply(prefix + ".ply");
  ply << "ply\nformat ascii 1.0\ncomment metres; rectified left optical X right Y down Z forward\n"
      << "element vertex " << count << "\nproperty float x\nproperty float y\nproperty float z\n"
      << "property uchar red\nproperty uchar green\nproperty uchar blue\nend_header\n";
  ply << std::setprecision(9);
  for (size_t i = 0; i < disparity.size(); ++i) {
    const double d = cb_fp16_to_float(disparity[i]);
    if (!std::isfinite(d) || d <= 0) continue;
    float xyz[3];
    if (cb_disparity_to_point(&frame,float(i%frame.width),float(i/frame.width),float(d),xyz)) continue;
    const double distance=std::sqrt(double(xyz[0])*xyz[0]+double(xyz[1])*xyz[1]+double(xyz[2])*xyz[2]);
    if (distance < 0.03 || distance > 10) continue;
    ply << xyz[0] << ' ' << xyz[1] << ' ' << xyz[2];
    for (int c = 0; c < 3; ++c) ply << ' ' << unsigned(color(rgb[i], c));
    ply << '\n';
  }
  std::ofstream meta(prefix + ".json");
  meta << std::setprecision(9) << "{\n  \"frame_id\": \"" << frame.frame_id
       << "\",\n  \"timestamp_monotonic_ns\": \"" << frame.timestamp_monotonic_ns
       << "\",\n  \"width\": " << frame.width << ",\n  \"height\": " << frame.height
       << ",\n  \"focal_px\": " << frame.focal_px << ",\n  \"cx\": " << frame.cx
       << ",\n  \"cy\": " << frame.cy << ",\n  \"baseline_metres\": " << frame.baseline_metres
       << ",\n  \"mode_id\": " << frame.mode_id
       << ",\n  \"projection\": " << frame.projection
       << ",\n  \"fy\": " << frame.fy
       << ",\n  \"vertical_fov_deg\": " << frame.vertical_fov_deg
       << ",\n  \"disparity_format\": " << frame.disparity_format << "\n}\n";
  if (!f16 || !ppm || !ply || !meta) throw std::runtime_error("Cannot write output files");
  std::cout << "DISPARITY " << frame.width << 'x' << frame.height << " timestamp_ns="
            << frame.timestamp_monotonic_ns << " points=" << count << " prefix=" << prefix << '\n';
}
int main(int argc, char **argv) {
  try {
    const std::string action = argc > 1 ? argv[1] : "help";
    if (action == "help" || action == "--help") {
      std::cout << "cambold_example info | imu [seconds] | image FILE.jpg | depth PREFIX\n";
      return 0;
    }
    if (action != "info" && action != "imu" && action != "image" && action != "depth")
      throw std::runtime_error("Unknown action; use --help");
    if ((action == "image" || action == "depth") && argc != 3)
      throw std::runtime_error("A file path or output prefix is required");
    Device device;
    if (action == "info") {
      std::lock_guard<std::mutex> lock(device.state.mutex);
      const auto &c = device.state.config; const auto &s = device.state.stereo;
      std::cout << "Connected\nfps=" << c.frame_rate << " imu=" << c.imu_output_enabled << " frame_rates=";
      for (uint32_t i = 0; i < c.frame_rate_count; ++i) std::cout << c.frame_rates[i] << ' ';
      std::cout << "\nstereo_flags=" << s.flags << " focal_px=" << s.focal_px << " message=" << s.message << '\n';
      for (uint32_t i = 0; i < s.mode_count; ++i)
        std::cout << "mode=" << s.modes[i].id << ' ' << s.modes[i].width << 'x' << s.modes[i].height << " max_fps=" << s.modes[i].max_fps << '\n';
    } else if (action == "image") save_jpeg(argv[2]);
    else if (action == "imu") read_imu(device, argc > 2 ? std::max(1, std::min(60, std::stoi(argv[2]))) : 5);
    else save_depth(device, argv[2]);
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
