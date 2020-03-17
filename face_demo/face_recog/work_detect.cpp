/*
-- (c) Copyright 2018 Xilinx, Inc. All rights reserved.
--
-- This file contains confidential and proprietary information
-- of Xilinx, Inc. and is protected under U.S. and
-- international copyright and other intellectual property
-- laws.
--
-- DISCLAIMER
-- This disclaimer is not a license and does not grant any
-- rights to the materials distributed herewith. Except as
-- otherwise provided in a valid license issued to you by
-- Xilinx, and to the maximum extent permitted by applicable
-- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
-- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
-- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
-- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
-- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
-- (2) Xilinx shall not be liable (whether in contract or tort,
-- including negligence, or under any other theory of
-- liability) for any loss or damage of any kind or nature
-- related to, arising under or in connection with these
-- materials, including for any direct, or any indirect,
-- special, incidental, or consequential loss or damage
-- (including loss of data, profits, goodwill, or any type of
-- loss or damage suffered as a result of any action brought
-- by a third party) even if such damage or loss was
-- reasonably foreseeable or Xilinx had been advised of the
-- possibility of the same.
--
-- CRITICAL APPLICATIONS
-- Xilinx products are not designed or intended to be fail-
-- safe, or for use in any application requiring fail-safe
-- performance, such as life-support or safety devices or
-- systems, Class III medical devices, nuclear facilities,
-- applications related to the deployment of airbags, or any
-- other applications that could lead to death, personal
-- injury, or severe property or environmental damage
-- (individually and collectively, "Critical
-- Applications"). Customer assumes the sole risk and
-- liability of any use of Xilinx products in Critical
-- Applications, subject only to applicable laws and
-- regulations governing limitations on product liability.
--
-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
-- PART OF THIS FILE AT ALL TIMES.
*/
#include <glog/logging.h>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include "work_detect.hpp"

using namespace std;
namespace xilinx {
namespace demo {
std::shared_ptr<Result> transform_detect(std::shared_ptr<Result> input, 
        xilinx::facedetect::FaceDetectResult *dpu_output) {
  uint64_t lid = 0;
  auto ptr = dynamic_cast<FrameResult *>(input.get());
  if (!ptr->results_.empty()) {
    ptr->results_.clear();
  }

  auto sequence = ptr->frame_info_.frame_id;
  auto frame_width = ptr->frame_info_.mat.cols;
  auto frame_height = ptr->frame_info_.mat.rows;
  LOG(INFO) << "Frame :" << sequence
            << " detect result :" << dpu_output->rects.size() << " face(s)";
  for (auto& r : dpu_output->rects) {
    uint64_t local_id = lid++;
    auto score = r.score;

    cv::Rect_<int> bbox(r.x * frame_width, 
                        r.y * frame_height, 
                        r.width * frame_width,
                        r.height * frame_height);
    if (bbox.x < 0 || bbox.y < 0 ||
        bbox.width <= 0 || bbox.height <= 0 ||
        bbox.x + bbox.width > frame_width || bbox.y + bbox.height > frame_height) {
      LOG(WARNING) << "Invalid bbox: " << bbox;
      continue;
    }
    LOG(INFO) << "Detected bbox: " << bbox;
    ptr->results_.emplace_back(
                std::make_shared<FaceDetTrackResult>(ptr->getImage(), bbox, score, sequence, local_id));
  }
  return input;  
}

std::shared_ptr<Result> WorkDetect::preprocess(std::shared_ptr<Result> input) {
  if (!input) {
    return nullptr;
  }
  auto ptr = dynamic_cast<FrameResult *>(input.get());  
  auto sequence = ptr->frame_info_.frame_id;
  if (tracker_) {
    bool add_det_start_success = tracker_->addDetStart(sequence);
    if (add_det_start_success) {
      LOG(INFO) << "Succeeded to add det start for frame " << sequence;
    } else {
      LOG(ERROR) << "Failed to add det start for frame " << sequence;
      return nullptr;
    }
  }
  return input;
}

std::shared_ptr<Result> WorkDetect:: postprocess( 
        std::shared_ptr<Result> input) {
  if (!input) {
    return nullptr;
  }
  auto ptr = dynamic_cast<FrameResult *>(input.get());  
  auto sequence = ptr->frame_info_.frame_id;
  if (tracker_) {
    tracker_->setDetEnd(sequence);
    LOG(INFO) << "Succeeded to add det end for frame " << sequence;
  }
  return input;
}

}
}
