#include <gtest/gtest.h>

#include <memory>

#include "Overlay.hpp"

class OverlayTest : public ::testing::Test {
  protected:
  void SetUp() override { overlay = std::make_unique<Overlay>(); }

  std::unique_ptr<Overlay> overlay;
};

TEST_F(OverlayTest, FindsClosestDetectionByTimestamp)
{
  std::vector<Detection> detections;

  detections.push_back({100 * GST_MSECOND, {{10, 10, 50, 50}}});

  detections.push_back({200 * GST_MSECOND, {{20, 20, 100, 100}}});

  detections.push_back({400 * GST_MSECOND, {{30, 30, 150, 150}}});

  overlay->setDetections(std::move(detections));

  auto result =
      overlay->getDetectionsWithLowestTimeDifference(210 * GST_MSECOND, 50);

  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->pts, 200 * GST_MSECOND);

  EXPECT_EQ(result->boxes[0].x, 20);
}

TEST_F(OverlayTest, ReturnsNothingWhenTimestampTooFarAway)
{
  std::vector<Detection> detections;

  detections.push_back({100 * GST_MSECOND, {{10, 10, 50, 50}}});

  overlay->setDetections(std::move(detections));

  auto result =
      overlay->getDetectionsWithLowestTimeDifference(500 * GST_MSECOND, 50);

  EXPECT_FALSE(result.has_value());
}

TEST_F(OverlayTest, ChoosesSmallestDifference)
{
  std::vector<Detection> detections;

  detections.push_back({90 * GST_MSECOND, {{1, 1, 10, 10}}});

  detections.push_back({120 * GST_MSECOND, {{2, 2, 20, 20}}});

  detections.push_back({150 * GST_MSECOND, {{3, 3, 30, 30}}});

  overlay->setDetections(std::move(detections));

  auto result =
      overlay->getDetectionsWithLowestTimeDifference(130 * GST_MSECOND, 50);

  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->pts, 120 * GST_MSECOND);
}

int main(int argc, char **argv)
{
  gst_init(&argc, &argv);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
