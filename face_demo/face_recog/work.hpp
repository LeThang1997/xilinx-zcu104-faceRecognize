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
#include "demo_inner.hpp"
#include "data.hpp"
#include "filter.hpp"

namespace xilinx {
namespace demo {

struct Work {
  Work(const std::string &info = "Work"):info_(info) {}
  explicit Work(std::unique_ptr<Filter> &&filter, const std::string &info = "Work")
        :filter_(std::move(filter)), info_(info) {
  } 
  Work(const Work&) = delete;
  Work &operator =(const Work&) = delete;

  virtual ~Work() {}
  virtual std::shared_ptr<Result> run(std::shared_ptr<Result> input) {
    if (!input) {
      return nullptr;
    } 
    auto output = preprocess(input);
    output = process(output); 
    return postprocess(output);
  }

  virtual std::shared_ptr<Result> process(std::shared_ptr<Result> input) {
    auto output = input;
    if (filter_ && output->valid() && check(output)) {
      output = ((FilterWithData *)(filter_.get()))->runWithResult(output); 
    } else {
      output = nullptr;
    }
    return output;
  }

  virtual std::shared_ptr<Result> preprocess(std::shared_ptr<Result> input) {
    return input;
  }
  virtual std::shared_ptr<Result> postprocess(std::shared_ptr<Result> input) {
    return input;
  }

  virtual bool check(std::shared_ptr<Result> input) {
    return true;
  }
  virtual std::string info () {
    return info_;
  }
  std::unique_ptr<Filter> filter_;
  std::string info_;
};

class WorkCrops: public Work {
public:
  WorkCrops(const std::string &info = "Work"):Work(info) {}
  explicit WorkCrops(std::unique_ptr<Filter> &&filter, const std::string &info = "Work")
        :Work(std::move(filter), info) {
  }
  virtual ~WorkCrops() {}
   
  virtual std::shared_ptr<Result> process(std::shared_ptr<Result> input) override {
    auto output = input;
    auto sequence =  (dynamic_cast<FrameResult *>(output.get()))->frame_info_.frame_id;

    if (filter_) {
      LOG(INFO) << "Frame: " << sequence
                << " start " << info();
      for (auto &r : (dynamic_cast<FrameResult *>(output.get()))->results_) {
        auto face  = (dynamic_cast<FaceDetTrackResult *>(r.get()));
        if (!face->valid()) {
          LOG(INFO) << "Frame: " << sequence
                    << " face local id: " << face->lid_
                    << " not valid, no need to run "
                    << info();
          continue;
        }

        LOG(INFO) << "Frame: " << sequence
                  << " face id: " << face->gid_
                  << " rect: " << face->bbox_;

        if (check(r)) {
          auto result = ((FilterWithData *)(filter_.get()))->runWithResult(r);
          r = result;
        } else {
          LOG(INFO) << "Frame: " << sequence
                    << " face id: " << face->gid_
                    << " check param not OK, no need to run "
                    << info();
        } 
      }
      LOG(INFO) << "Frame: " << sequence
                 << " stop " << info();
    }
    return output;
  } 

};

}
}
