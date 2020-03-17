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
#include "work_track.hpp"

using namespace std;
namespace xilinx {
namespace demo {

std::shared_ptr<Result> WorkTrack::preprocess(std::shared_ptr<Result> input) {
  if (!input) {
    return nullptr;
  }

  auto ptr = dynamic_cast<FrameResult *>(input.get());  
  auto sequence = ptr->frame_info_.frame_id;
  auto &det_results = ptr->results_;
  auto frame_width = ptr->frame_info_.mat.cols;
  auto frame_height = ptr->frame_info_.mat.rows;

  // Build track input
  input_characts_.clear();

  uint64_t lid = 0;
  for (auto& r : det_results) {
    auto p = dynamic_cast<FaceDetTrackResult *>(r.get());
    uint64_t local_id = lid++;
    auto bbox = cv::Rect_<float>((float(p->bbox_.x)) / frame_width,
                                (float(p->bbox_.y)) / frame_height,
                                (float(p->bbox_.width)) / frame_width,
                                (float(p->bbox_.height)) / frame_height);

    LOG(INFO) << "Frame: " << sequence 
               << " build input_characts: " << bbox
               << ", " << p->score_ << ", " << local_id;
    input_characts_.emplace_back(bbox, p->score_, -1, local_id);
  }

  return input;
}

// Do Track
std::shared_ptr<Result> WorkTrack::process(std::shared_ptr<Result> input) {
  if (!input) {
    return nullptr;
  }
  auto ptr = dynamic_cast<FrameResult *>(input.get());  
  auto sequence = ptr->frame_info_.frame_id;
  // Track
  // Lock
  bool lock_success = tracker_->setTrackLock(sequence);
  if (lock_success) {
    LOG(INFO) << "Succeeded to set track lock for frame " << sequence;
  } else {
    LOG(ERROR) << "Failed to set track lock for frame " << sequence;
    tracker_->setDetTimeout(sequence);
    return nullptr;
  }

  // Call track and get output
  output_characts_ = tracker_->trackWithoutLock(sequence, input_characts_, true, true);

  // Unlock
  bool unlock_success = tracker_->releaseTrackLock(sequence);
  //tracker_->printState();
  if (unlock_success) {
    LOG(INFO) << "Succeeded to release track lock for frame " << sequence;
  } else {
    LOG(ERROR) << "Failed to release track lock for frame " << sequence;
  }
  LOG(INFO) << "Track results of frame " << sequence << ": "
               << input_characts_.size() << " objects input, "
               << output_characts_.size() << " objects tracked!";
  return input;
}

/// Transform OutputCharact to FaceDetTrackResult(s)
std::shared_ptr<Result> WorkTrack::postprocess(std::shared_ptr<Result> input) {
  if (!input) {
    return nullptr;
  }

  auto output = input;
  auto ptr = dynamic_cast<FrameResult *>(input.get());  
  auto sequence = ptr->frame_info_.frame_id;
  auto frame_width = ptr->frame_info_.mat.cols;
  auto frame_height = ptr->frame_info_.mat.rows;
  
  // Construct results
  for (auto& r: output_characts_) {
    auto& gid = get<0>(r);
    //auto& score = get<2>(r);
    auto& lid = get<4>(r);
    auto& bbox = get<1>(r);
    bbox.x *= frame_width;
    bbox.y *= frame_height;
    bbox.width *= frame_width;
    bbox.height *= frame_height;
    LOG(INFO) << "Frame: " << sequence
              << ", track bbox: " << bbox
              << ", gid: " << gid
              << ", lid: " << lid;
    // lid < 0 means object is undetected and created by tracking
    if (lid >= 0) {
      auto f = dynamic_cast<FaceDetTrackResult *>(ptr->results_[lid].get()); 
      f->gid_ = gid;
    }
  }

  for (auto &f : ptr->results_) {
    auto real_p = dynamic_cast<FaceDetTrackResult *>(f.get());    
    if (real_p->gid_ == UINT64_MAX) {
      LOG(INFO) << "Frame: " << sequence << " lid: " << real_p->lid_ 
                << " not tracked, will set invalid";
      real_p->valid_ = false;
    }
  } 
  return output;
}

}
}
