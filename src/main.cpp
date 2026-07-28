#include <gst/gst.h>

#include "VideoSource.hpp"

int main(int argc, char* argv[])
{
  gst_init(&argc, &argv);

  Overlay overlay;
  srand((unsigned)time(NULL));

  VideoSource source({{"videotestsrc", "source"},
                      {"videoconvert", "convert1"},
                      {"videoconvert", "convert2"},
                      {"autovideosink", "sink"}},
                     overlay);

  // Test-Rechteck
  auto randomnumber = [](int range) -> int {
    return -range + (rand() % range);
  };

  /*
  bool run = true;
  std::thread t1([&]() {
    while (run) {
      overlay.setDetections(
          {7600 * GST_MSECOND,
           {{50 + randomnumber(25), 50 + randomnumber(25),
             100 + randomnumber(50), 100 + randomnumber(50)}}});
    }
  });
  */

  bool run = true;
  std::thread t1([&]() {
    uint64_t timestamp = 0;

    while (run) {
      std::vector<Detection> list;
      timestamp += 10;
      for (int i = 0; i < 10; i++) {
        list.push_back({(timestamp + randomnumber(15)) * GST_MSECOND,
                        {{50 + randomnumber(25), 50 + randomnumber(25),
                          100 + randomnumber(50), 100 + randomnumber(50)}}});
      }

      overlay.setDetections(std::move(list));
      std::cout << "After:" << timestamp << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

  g_main_loop_run(loop);

  run = false;
  t1.join();
  g_main_loop_unref(loop);

  return 0;
};
