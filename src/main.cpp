#include <gst/gst.h>

#include <chrono>
#include <cstdlib>
#include <thread>

#include "DummyCamera.hpp"
#include "VideoSource.hpp"

int main(int argc, char* argv[])
{
  gst_init(&argc, &argv);

  Overlay overlay;

  VideoSource source({{"appsrc", "camera-source"},
                      {"videoconvert", "convert1"},
                      {"videoconvert", "convert2"},
                      {"autovideosink", "sink"}},
                     overlay);

  DummyCamera camera(640, 480, 30);

  bool run = true;

  //
  // Dummy Kamera Thread
  //
  std::thread cameraThread([&]() {
    camera.start([&](const auto& frame, GstClockTime pts) {
      source.pushFrame(frame, pts);
    });
  });

  //
  // Dummy Detection Thread
  //
  auto randomnumber = [](int range) -> int {
    return -range + (rand() % range);
  };

  std::thread detectionThread([&]() {

    while (run) {
      std::vector<Detection> list;

      auto now = std::chrono::steady_clock::now().time_since_epoch();

      GstClockTime pts =
          std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

      for (int i = 0; i < 10; i++) {
        list.push_back({(pts + randomnumber(15)),
                        {{50 + randomnumber(25), 50 + randomnumber(25),
                          100 + randomnumber(50), 100 + randomnumber(50)}}});
      }

      overlay.setDetections(std::move(list));

      std::cout << "Detection timestamp: " << pts << " ms" << std::endl;

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

  g_main_loop_run(loop);

  //
  // Shutdown
  //
  run = false;

  camera.stop();

  cameraThread.join();

  detectionThread.join();

  g_main_loop_unref(loop);

  return 0;
}
