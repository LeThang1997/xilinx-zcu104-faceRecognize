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
#include <memory>
#include <utility>
#include <glog/logging.h>
#include <iostream>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <xilinx/facerecog/facerecog.hpp>
#include "work.hpp"

using namespace std;
namespace xilinx {
namespace demo {

std::shared_ptr<Result> transform_recog(std::shared_ptr<Result> input, 
        xilinx::facerecog::FaceRecogFixedResult *dpu_output);

using recog_model_type_t= xilinx::facerecog::FaceRecog;
using recog_dpu_result_t = xilinx::facerecog::FaceRecogFixedResult;

template <>
struct DpuFilterWithData<recog_model_type_t, recog_dpu_result_t> : public FilterWithData {
  using ProcessorType = std::shared_ptr<Result>(std::shared_ptr<Result>, recog_dpu_result_t *);

  DpuFilterWithData(std::unique_ptr<recog_model_type_t>&& dpu_model,
            const ProcessorType& processor)
      : FilterWithData{}, dpu_model_{std::move(dpu_model)}, 
        processor_{processor} {
  }

  virtual ~DpuFilterWithData() {
  }

  virtual std::shared_ptr<Result> runWithResult(std::shared_ptr<Result> input) override {
    auto real_ptr = dynamic_cast<FaceCaptureResult *>(input.get());
    auto &mat = real_ptr->full_image_;
    auto frame_width = mat.cols;
    auto frame_height = mat.rows;
    auto &bbox = real_ptr->bbox_;
    LOG(INFO) << "Face id: " << real_ptr->gid_
              << " will run recog: bbox " << bbox
              << " quality: " << real_ptr->quality_;
    auto recog_object = std::shared_ptr<Result>(new FaceRecogResult(*real_ptr));
    auto rect_pair = xilinx::facerecog::FaceRecog::expand_and_align(frame_width, frame_height,
              bbox.x, bbox.y, bbox.width, bbox.height,
              0.2, 0.2, 8, 8);
    LOG(INFO) << "Recog pair: " << rect_pair.first << ", "
               << rect_pair.second;
    auto dpu_result = dpu_model_->run_fixed(mat(rect_pair.first), 
                                            rect_pair.second.x,
                                            rect_pair.second.y,
                                            rect_pair.second.width,
                                            rect_pair.second.height);
    return processor_(recog_object, &dpu_result);
  }

  cv::Mat run(cv::Mat& image) override {
    return image;
  }
  std::unique_ptr<recog_model_type_t> dpu_model_;
  const ProcessorType &processor_;
}; 

static bool check_internal(std::shared_ptr<Result> input) {
  bool ok = true;

  auto real_ptr = dynamic_cast<FaceCaptureResult *>(input.get());
  if (real_ptr->quality_ < 0.1) {
    LOG(WARNING) << "Face id: " << real_ptr->gid_
                 << " has low quality : " << real_ptr->quality_
                 << ", no need to recog";
    real_ptr->valid_ = false;
    ok = false; 
  }
  return ok;
}

class WorkRecog: public Work{
public:
  static std::unique_ptr<Filter> createFilter() {
    return std::unique_ptr<Filter>(
            new DpuFilterWithData<facerecog::FaceRecog, facerecog::FaceRecogFixedResult>
                    (facerecog::FaceRecog::create(), transform_recog)); 
  }

  explicit WorkRecog(std::unique_ptr<Filter> &&filter):Work{std::move(filter), std::string("WorkRecog")} {
  }

  virtual bool check(std::shared_ptr<Result> input) {
    return check_internal(input);
  }

};

class WorkRecogFrame: public WorkCrops {
public:
  static std::unique_ptr<Filter> createFilter() {
    return std::unique_ptr<Filter>(
            new DpuFilterWithData<facerecog::FaceRecog, facerecog::FaceRecogFixedResult>
                    (facerecog::FaceRecog::create(), transform_recog)); 
  }

  explicit WorkRecogFrame(std::unique_ptr<Filter> &&filter):WorkCrops{std::move(filter), std::string("WorkRecogFrame")} {
  }

  virtual bool check(std::shared_ptr<Result> input) {
    return check_internal(input);
  }
 
  virtual std::shared_ptr<Result> postprocess(std::shared_ptr<Result> input) {
    for (auto &r : (dynamic_cast<FrameResult *>(input.get()))->results_) {
      auto f = dynamic_cast<FaceCaptureResult *>(r.get());

      if (!f->valid()) {
        continue;
      }
      /*------------------------------------------------------------------------
      * Modifed : ThangLmb
      --------------------------------------------------------------------------*/
      //cv::imwrite("imagge.jpg",f->full_image_);

      cv::rectangle(f->full_image_, cv::Rect{cv::Point(f->bbox_.x,  f->bbox_.y),
                                  cv::Size{(int)(f->bbox_.width),
                                           (int)(f->bbox_.height)}},
                  0xff);
      cv::Scalar color{0, 255, 255};
      // Show face quality
      
      cv::putText(f->full_image_, to_string(f->quality_), 
                cv::Point{(int)(f->bbox_.x), (int)(f->bbox_.y)}, 
                cv::FONT_HERSHEY_PLAIN, 1, color, 1, 1, 0);

      //Show tracked id
      cv::putText(f->full_image_, to_string(f->gid_), 
                cv::Point{(int)(f->bbox_.x + 10), (int)(f->bbox_.y + 10)}, 
                cv::FONT_HERSHEY_PLAIN, 1, color, 1, 1, 0);
     /*------------------------------------------------------------------------
      * Modifed : ThangLmb
      --------------------------------------------------------------------------*/
     // std::cout<<"Result 1:   " <<to_string(f->quality_)<<endl;
      //std::cout<<"Rsult 2:   " << to_string(f->gid_) <<endl;
    }
    return input;
  }

};

}
}
