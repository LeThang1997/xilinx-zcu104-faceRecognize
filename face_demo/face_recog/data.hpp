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
#pragma once 
#include <opencv2/opencv.hpp>
#include <cstdint>
#include <type_traits>
#include <xilinx/facequality/facequality.hpp>
#include <xilinx/facerecog/facerecog.hpp>
#include "demo_inner.hpp"

namespace xilinx {
namespace demo {

struct Result {
  Result():valid_{true}{}
  virtual ~Result(){}
  virtual bool valid() {return valid_;}
  virtual cv::Mat getImage() {
    return cv::Mat();
  } 
  bool valid_;
};

struct FrameResult : public Result {
  FrameResult(){}
  FrameResult(const FrameInfo &frame_info):frame_info_(frame_info) {}
  virtual cv::Mat getImage() override {
    return frame_info_.mat;
  }

  FrameInfo frame_info_;
  std::vector<std::shared_ptr<Result>> results_;
};

struct FaceDetTrackResult : public Result {
  FaceDetTrackResult(const cv::Mat &full_image, const cv::Rect_<int32_t> bbox, 
                     uint64_t frame_id, uint64_t lid, uint64_t gid = UINT64_MAX)
        :full_image_(full_image), bbox_(bbox), score_(0.f), 
                     frame_id_(frame_id), lid_(lid), gid_(gid) {}
  FaceDetTrackResult(const cv::Mat &full_image, const cv::Rect_<int32_t> bbox, float score,
                     uint64_t frame_id, uint64_t lid, uint64_t gid = UINT64_MAX)
        :full_image_(full_image), bbox_(bbox), score_(score), 
                     frame_id_(frame_id), lid_(lid), gid_(gid) {}

  virtual cv::Mat getImage() override {
    cv::Rect_<int32_t> b(bbox_.x, bbox_.y, 
                     bbox_.width, bbox_.height);
    return full_image_(b);
  }

  cv::Mat full_image_;
  cv::Rect_<int32_t> bbox_;
  float score_; 
  uint64_t frame_id_;
  uint64_t lid_;
  uint64_t gid_;
};

struct FaceCaptureResult : public FaceDetTrackResult {
  FaceCaptureResult(const FaceDetTrackResult &input, float quality)
        :FaceDetTrackResult(input), quality_(quality) {}
  FaceCaptureResult(FaceDetTrackResult &&input, float quality)
        :FaceDetTrackResult(std::move(input)), quality_(quality) {}

  float quality_;
};

struct FaceRecogResult : public FaceCaptureResult{
  FaceRecogResult(const FaceCaptureResult &input) 
                  :FaceCaptureResult(input) {
  }

  FaceRecogResult(const FaceCaptureResult &input, 
                  xilinx::facerecog::FaceRecogFixedResult *recog_result)
                  :FaceCaptureResult(input), 
                   gender_(recog_result->gender),
                   age_(recog_result->age),
                   feature_(std::move(recog_result->feature)) {
                   
  }

  uint8_t gender_; 
  uint8_t age_;
  std::unique_ptr<std::array<int8_t, 512>> feature_;
};

struct FaceItem {
  FaceItem(){}
  FaceItem(const std::string &name, const std::vector<int8_t> &feature)
          : name_(name), feature_(feature){}
  FaceItem(const std::string &name, cv::Mat &mat, const std::vector<int8_t> &feature)
          : name_(name), mat_(mat), feature_(feature){}

  std::string name_;
  cv::Mat mat_;
  std::vector<int8_t> feature_;
};

struct FaceCompareResult : public Result {
  FaceCompareResult() {}
  FaceCompareResult(const FaceRecogResult &input, const FaceItem &item, float similarity) 
                 :image_(item.mat_), name_(item.name_), 
                  gid_(input.gid_), similarity_(similarity), quality_(input.quality_),
                  gender_(input.gender_), age_(input.age_) {
  }

  cv::Mat image_;
  std::string name_;
  uint64_t gid_;
  float similarity_;
  float quality_;
  uint8_t gender_;
  uint8_t age_;
};


} // end of namespace demo
} // end of namespace xilinx
