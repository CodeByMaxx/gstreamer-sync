#pragma once

#include <cairo.h>
#include <gst/gst.h>

#include <algorithm>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

struct BoundingBox {
  int x;
  int y;
  int width;
  int height;
};

struct Detection {
  GstClockTime pts = GST_CLOCK_TIME_NONE;
  std::vector<BoundingBox> boxes;
};

template <class T>
class FixedDeque {
  public:
  explicit FixedDeque(size_t size) : maxSize(size) {}

  auto begin() { return dq.begin(); }

  auto end() { return dq.end(); }

  auto begin() const { return dq.begin(); }

  auto end() const { return dq.end(); }

  void push_back(T&& value)
  {
    if (dq.size() >= maxSize) {
      dq.pop_front();
    }

    dq.push_back(std::move(value));
  }

  size_t size() const { return dq.size(); }

  private:
  std::deque<T> dq;

  size_t maxSize;
};

class Overlay {
  public:
  explicit Overlay(int size = 50) : detections(size)
  {
    element = gst_element_factory_make("cairooverlay", "box-overlay");

    if (!element) {
      throw std::runtime_error("Could not create cairooverlay");
    }

    gulong signalId =
        g_signal_connect(element, "draw", G_CALLBACK(drawCallback), this);

    std::cout << "draw signal id: " << signalId << std::endl;
  }

  GstElement* getElement() const { return element; }

  void setDetections(std::vector<Detection>&& list)
  {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto& detection : list) {
      std::cout << "Overlay received detection: " << detection.pts / GST_MSECOND
                << " ms" << std::endl;

      detections.push_back(std::move(detection));
    }
  }

  //
  // Unit-Test API
  //
  std::optional<Detection> getDetectionsWithLowestTimeDifference(
      GstClockTime timestamp, int offsetMs)
  {
    std::lock_guard<std::mutex> lock(mutex);

    return findDetection(timestamp, offsetMs);
  }

  private:
  static void drawCallback(GstElement*, cairo_t* cr, guint64 timestamp, guint64,
                           gpointer userData)
  {
    auto self = static_cast<Overlay*>(userData);

    self->draw(cr, timestamp);
  }

  void draw(cairo_t* cr, GstClockTime timestamp)
  {
    std::cout << "DRAW timestamp: " << timestamp / GST_MSECOND << " ms"
              << std::endl;

    auto detection = getDetectionsWithLowestTimeDifference(timestamp, 200);

    if (detection) {
      std::cout << "BOX FOUND" << std::endl;

      lastDetection = detection;
    }

    if (!lastDetection) {
      return;
    }

    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

    cairo_set_line_width(cr, 3.0);

    for (const auto& box : lastDetection->boxes) {
      cairo_rectangle(cr, box.x, box.y, box.width, box.height);

      cairo_stroke(cr);
    }
  }

  std::optional<Detection> findDetection(GstClockTime timestamp, int offsetMs)
  {
    std::optional<Detection> result;

    GstClockTime best = GST_CLOCK_TIME_NONE;

    for (const auto& detection : detections) {
      if (!GST_CLOCK_TIME_IS_VALID(detection.pts)) {
        continue;
      }

      GstClockTime diff = timestamp > detection.pts ? timestamp - detection.pts
                                                    : detection.pts - timestamp;

      if (!GST_CLOCK_TIME_IS_VALID(best) || diff < best) {
        best = diff;

        result = detection;
      }
    }

    if (result && best < offsetMs * GST_MSECOND) {
      return result;
    }

    return std::nullopt;
  }

  private:
  GstElement* element = nullptr;

  FixedDeque<Detection> detections;

  std::mutex mutex;

  std::optional<Detection> lastDetection;
};
