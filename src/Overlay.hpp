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

class PositionGenerator {};

struct BoundingBox {
  int x;
  int y;
  int width;
  int height;
};

struct FrameInfo {
  GstClockTime pts = GST_CLOCK_TIME_NONE;
};

struct Detection {
  GstClockTime pts = GST_CLOCK_TIME_NONE;
  std::vector<BoundingBox> boxes;
};

template <class T>
class FixedDeque {
  private:
  std::deque<T> dq;
  size_t maxSize;

  public:
  explicit FixedDeque(size_t size) : maxSize(size) {}

  auto begin() { return dq.begin(); }

  auto end() { return dq.end(); }

  auto begin() const { return dq.begin(); }

  auto end() const { return dq.end(); }

  T& at(size_t index) { return dq.at(index); }

  const T& at(size_t index) const { return dq.at(index); }

  void push_back(T&& value)
  {
    if (dq.size() >= maxSize) {
      dq.pop_front();
    }

    dq.push_back(std::move(value));
  }

  void clear() { dq.clear(); }

  size_t size() const { return dq.size(); }
};

class Overlay {
  public:
  explicit Overlay(int size = 20) : detections(size)
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

  void setDetections(std::vector<Detection>&& detection_)
  {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto& detection : detection_) {
      detections.push_back(std::move(detection));
    }

    // std::cout << "detections: " << detections.size() << std::endl;
  }

  void orderDetections()
  {
    std::lock_guard<std::mutex> lock(mutex);

    std::sort(detections.begin(), detections.end(),
              [](const Detection& left, const Detection& right) {
                return left.pts < right.pts;
              });
  }

  std::optional<Detection> getDetectionsWithLowestTimeDifference(
      GstClockTime time, int offset)
  {
    std::lock_guard<std::mutex> lock(mutex);

    std::optional<Detection> result;

    GstClockTime smallestDiff = GST_CLOCK_TIME_NONE;

    for (const auto& detection : detections) {
      if (!GST_CLOCK_TIME_IS_VALID(time) ||
          !GST_CLOCK_TIME_IS_VALID(detection.pts)) {
        continue;
      }

      GstClockTime diff =
          (time > detection.pts) ? time - detection.pts : detection.pts - time;

      if (!GST_CLOCK_TIME_IS_VALID(smallestDiff) || diff < smallestDiff) {
        smallestDiff = diff;
        result = detection;
      }
    }

    if (result && smallestDiff < offset * GST_MSECOND) {
      return result;
    }

    return std::nullopt;
  }

  void setTimestamp(GstClockTime pts)
  {
    std::lock_guard<std::mutex> lock(mutex);

    frameInfo.pts = pts;
  }

  private:
  static void drawCallback(GstElement* overlay, cairo_t* cr, guint64 timestamp,
                           guint64 duration, gpointer user_data)
  {
    auto self = static_cast<Overlay*>(user_data);

    self->draw(cr, timestamp);
  }

  void draw(cairo_t* cr, guint64 timestamp)
  {
    std::optional<Detection> detection;

    {
      std::lock_guard<std::mutex> lock(mutex);

      detection = getDetectionUnsafe(timestamp, 1000);
    }

    if (detection) {
      std::cout << "Update detection" << std::endl;
      olddetection = detection;
    }

    if (!olddetection) {
      return;
    }

    for (const auto& box : olddetection->boxes) {
      cairo_rectangle(cr, box.x, box.y, box.width, box.height);

      cairo_set_line_width(cr, 3.0);

      cairo_stroke(cr);
    }
  }

  // Wird nur mit gehaltenem Mutex aufgerufen
  std::optional<Detection> getDetectionUnsafe(GstClockTime time, int offset)
  {
    std::optional<Detection> result;

    GstClockTime smallestDiff = GST_CLOCK_TIME_NONE;

    for (const auto& detection : detections) {
      if (!GST_CLOCK_TIME_IS_VALID(time) ||
          !GST_CLOCK_TIME_IS_VALID(detection.pts)) {
        continue;
      }

      GstClockTime diff =
          (time > detection.pts) ? time - detection.pts : detection.pts - time;

      if (!GST_CLOCK_TIME_IS_VALID(smallestDiff) || diff < smallestDiff) {
        smallestDiff = diff;
        result = detection;
      }
    }

    if (result && smallestDiff < offset * GST_MSECOND) {
      return result;
    }

    return std::nullopt;
  }

  private:
  GstElement* element = nullptr;

  FixedDeque<Detection> detections;

  std::mutex mutex;

  FrameInfo frameInfo;

  std::optional<Detection> olddetection;
};
