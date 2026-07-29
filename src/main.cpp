#include <gst/gst.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
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

  std::mutex timestampMutex;

  GstClockTime cameraTimestamp = 0;

  // erster Thread
  std::thread cameraThread([&]() {
    camera.start([&](const auto& frame, GstClockTime pts) {
      {
        std::lock_guard<std::mutex> lock(timestampMutex);

        cameraTimestamp = pts;
      }

      std::cout << "Kamera timestamp: " << pts / GST_MSECOND << " ms"
                << std::endl;

      source.pushFrame(frame, pts);
    });
  });

  // zweiter Thread
  std::thread detectionThread([&]() {
    std::mt19937 generator(std::random_device{}());

    std::uniform_int_distribution<int> delay(20, 80);

    std::uniform_int_distribution<int> position(-25, 25);

    while (run) {
      GstClockTime pts = 0;

      {
        std::lock_guard<std::mutex> lock(timestampMutex);

        pts = cameraTimestamp;
      }

      if (pts == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        continue;
      }

      int processingDelay = delay(generator);

      GstClockTime detectionTimestamp = pts + processingDelay * GST_MSECOND;

      BoundingBox box{50 + position(generator), 50 + position(generator), 100,
                      100};

      Detection detection{detectionTimestamp, {box}};

      overlay.setDetections(std::vector<Detection>{detection});

      std::cout << "Detection timestamp: " << detectionTimestamp / GST_MSECOND
                << " ms" << std::endl;

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

  g_main_loop_run(loop);

  run = false;

  camera.stop();

  cameraThread.join();

  detectionThread.join();

  g_main_loop_unref(loop);

  return 0;
}
