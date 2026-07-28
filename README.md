# GStreamer Cairo Overlay

A small C++ project demonstrating how to draw dynamic bounding boxes on a video stream using **GStreamer**, **Cairo**, and **cairooverlay**.

Bounding boxes are generated independently from the video pipeline and matched to the current video frame using the closest presentation timestamp (PTS).


---

## Features

* Draw bounding boxes using `cairooverlay`
* Timestamp-based detection matching
* Thread-safe communication between the detection thread and the drawing thread
* Fixed-size detection buffer
* Unit tests using GoogleTest

---

## Project Structure

```text
.
├── CMakeLists.txt
├── README.md
├── src
│   ├── main.cpp
│   └── Overlay.hpp
└── tests
    └── OverlayTest.cpp
```

---

## Requirements

The following packages are required:

* C++17 compatible compiler
* CMake (>= 3.16)
* GStreamer 1.0
* GStreamer Video
* Cairo
* GoogleTest

### Ubuntu / Debian

```bash
sudo apt update

sudo apt install \
    cmake \
    build-essential \
    pkg-config \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libcairo2-dev \
    libgtest-dev
```

---

## Build

```bash
mkdir build
cd build

cmake ..
make
```

---

## Run

```bash
./overlay
```

---

## Unit Tests

Build the project:

```bash
mkdir build
cd build

cmake ..
make
```

Run all tests:

```bash
ctest --verbose
```

or execute the test binary directly:

```bash
./overlay_test
```

---

## Detection Matching

Each detection contains a presentation timestamp (PTS).

```cpp
struct Detection
{
    GstClockTime pts;
    std::vector<BoundingBox> boxes;
};
```

When a video frame arrives, the overlay searches the detection buffer and selects the detection whose timestamp has the smallest absolute difference to the current frame timestamp.

If the smallest difference exceeds the configured threshold (for example 50 ms), no new detection is returned.

---

## Thread Model

```text
             Detection Thread
                     │
                     ▼
            setDetections()
                     │
              protected by mutex
                     │
                     ▼
             Fixed Detection Buffer
                     ▲
                     │
              protected by mutex
                     │
                     ▼
        GStreamer cairooverlay Draw Callback
                     │
                     ▼
             Draw Bounding Boxes
```

## License

This project is provided for educational and experimental purposes.

