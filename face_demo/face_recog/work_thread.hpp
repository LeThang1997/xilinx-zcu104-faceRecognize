/*
-- (c) Copyright 2018 Xilinx, Inc. All rights reserved.
--
-- This file contains confidential and proprietary namermation
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
#include <thread>
#include <signal.h>
#include <unistd.h>
#include "frame_queue.hpp"
#include "work.hpp"

namespace xilinx {
namespace demo {

struct WorkThread: public MyThread {
  WorkThread(std::vector<std::unique_ptr<Work>>&& works, 
             FrameQueue* queue_in,
             FrameQueue* queue_out, 
             const std::string& suffix,
             FrameQueue* queue_other = nullptr,
             bool split = 0)
      : MyThread{},
        split_{split},
        works_{std::move(works)},
        queue_in_{queue_in},
        queue_out_{queue_out},
        queue_other_{queue_other},
        suffix_{suffix} {}
  virtual ~WorkThread() {}
  virtual int run() override {
    std::shared_ptr<Result> frame_in;
    if (!queue_in_->pop(frame_in, std::chrono::milliseconds(10))) {
      return 0;
    }
    auto frame_out = frame_in;
    for (auto &w : works_) {
      LOG(INFO) << " [" << name() << "] "  
                << "Run work: " << w->info();
      frame_out = w->run(frame_out);
    }
    if (split_ && queue_other_) {
      auto p = dynamic_cast<FrameResult *>(frame_out.get());
      LOG(INFO) << " [" << name() << "] "  
                << "Frame :" << p->frame_info_.frame_id << " split";
      auto outs = split(frame_out);  
      for (auto &o : outs) {
        LOG(INFO) << " [" << name() << "] "  
                  << "Push to split queue:" << queue_other_->info();
        if (!queue_other_->push(o, std::chrono::milliseconds(2))) {
          LOG(INFO) << " [" << name() << "] "  
                    << "Push split queue error: queue is full";
        }
      }
    }

    if (frame_out) {
      while (!queue_out_->push(frame_out, std::chrono::milliseconds(10))) {
        LOG(INFO) << " [" << name() << "] "  
                  << "Try to push queue " << queue_out_->info()
                  << " again " << queue_out_->capacity() << " vs " << queue_out_->size();
        if (is_stopped()) {
          return -1;
        }
      }
    }
    return 0;
  }

  virtual std::vector<std::shared_ptr<Result>> split(std::shared_ptr<Result> frame_result) {
    auto p = dynamic_cast<FrameResult *>(frame_result.get());
    std::vector<std::shared_ptr<Result>> results;
    for (auto &r : p->results_) {
      auto real_p = (dynamic_cast<FaceDetTrackResult *>(r.get()));
      if (r->valid()) 
      {
        LOG(INFO) << " [" << name() << "] "  
                  << "Valid piece: gid = " << real_p->gid_; 
        results.emplace_back(r);
      } 
      else 
      {
        LOG(INFO) << " [" << name() << "] "  
                  << "Invalid piece: lid = " << real_p->lid_ 
                  << ", drop it";
      }
    }
    return results;
  }
  virtual std::string name() override { return std::string("T-Work-") + suffix_; }
private:
  bool split_;
  std::vector<std::unique_ptr<Work>> works_;
  FrameQueue* queue_in_;
  FrameQueue* queue_out_;
  FrameQueue* queue_other_;
  std::string suffix_;
};

} // end of demo
} // end of xilinx
