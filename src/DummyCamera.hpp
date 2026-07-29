#pragma once

#include <gst/gst.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

class DummyCamera {
  public:
  DummyCamera(int width, int height, int fps)
      : width(width), height(height), fps(fps)
  {
  }

  template <typename Callback>
  void start(Callback callback)
  {
    while (running) {
      auto frame = generateFrame();

      auto now = std::chrono::steady_clock::now().time_since_epoch();

      GstClockTime pts =
          std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

      std::cout << "Kamera timestamp: " << pts << std::endl;

      callback(frame, pts);

      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
    }
  }

  void stop() { running = false; }

  private:
      std::vector<uint8_t> generateFrame()
      {
        std::vector<uint8_t> frame(width * height * 3);

        static uint8_t color = 0;

        for (size_t i = 0; i < frame.size(); i += 3) {
          frame[i] = 255;    // R
          frame[i + 1] = 0;  // G
          frame[i + 2] = 0;  // B
        }

        return frame;
      }

  private:
  int width;
  int height;
  int fps;

  std::atomic<bool> running{true};
};
