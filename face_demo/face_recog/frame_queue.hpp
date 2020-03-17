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
#include <glog/logging.h>
#include <xilinx/base/queue/bounded_queue.hpp>
#include <string>
#include <memory>
#include "demo_inner.hpp"
#include "data.hpp"

namespace xilinx {
namespace demo {

class FrameQueue {
public:
  explicit FrameQueue(std::size_t capacity, const std::string &info)
                      : q_(capacity), info_(info) {
  }
  virtual ~FrameQueue(){} 
  virtual std::size_t capacity() const {
    return q_.capacity();
  }
  virtual std::size_t size() const {
    return q_.size();
  }
  virtual void push(const std::shared_ptr<Result> &new_value) {
    return q_.push(new_value);
  }
  virtual bool push(const std::shared_ptr<Result> &new_value, const std::chrono::milliseconds &rel_time) {
    return q_.push(new_value, rel_time);
  }
  virtual void pop(std::shared_ptr<Result> &value) {
    return q_.pop(value);
  }
  virtual bool pop(std::shared_ptr<Result> &value, const std::chrono::milliseconds &rel_time) {
    return q_.pop(value, rel_time);
  }
  virtual bool top(const std::shared_ptr<Result> &new_value, const std::chrono::milliseconds &rel_time) {
    return q_.push(new_value, rel_time);
  }

  virtual std::string info() {
    return info_; 
  }

private:
  xilinx::BoundedQueue<std::shared_ptr<Result>> q_;
  std::string info_;
};

class FrameQueueAdapter : public FrameQueue {
public:
  explicit FrameQueueAdapter(std::shared_ptr<queue_t> q, const std::string &info):FrameQueue(0, info), inner_q_(q) {}
  virtual std::shared_ptr<queue_t> getOriginal() {return inner_q_;}
  virtual ~FrameQueueAdapter(){} 
  virtual std::size_t capacity() const {
    return inner_q_->capacity();
  }
  virtual std::size_t size() const {
    return inner_q_->size();
  }
  virtual void push(const std::shared_ptr<Result> &new_value) {
    auto p = dynamic_cast<FrameResult *>(new_value.get());
    inner_q_->push(p->frame_info_);
    /*------------------------------------------------------------------------
      * Modifed : ThangLmb
      --------------------------------------------------------------------------*/
     // std::cout<<"Rsult 2:   " << to_string(p->gid_) <<endl;

  }
  virtual bool push(const std::shared_ptr<Result> &new_value, const std::chrono::milliseconds &rel_time) {
    auto p = dynamic_cast<FrameResult *>(new_value.get());
    return inner_q_->push(p->frame_info_, rel_time);
  }
  virtual void pop(std::shared_ptr<Result> &value) {
    FrameInfo v; 
    inner_q_->pop(v);
    value = std::shared_ptr<Result>(new FrameResult(v));
  }

  virtual bool pop(std::shared_ptr<Result> &value, const std::chrono::milliseconds &rel_time) {
    FrameInfo v; 
    bool ok = inner_q_->pop(v, rel_time);
    if (ok) {
      value = std::shared_ptr<Result>(new FrameResult(v));
    }
    return ok; 
  }

  virtual bool top(std::shared_ptr<Result> &value, const std::chrono::milliseconds &rel_time) {
    FrameInfo v; 
    bool ok = inner_q_->top(v, rel_time);
    if (ok) {
      value = std::shared_ptr<Result>(new FrameResult(v));
    }
    return ok; 
  }

private:
  std::shared_ptr<queue_t> inner_q_;
};

} // end of demo
} // end of xilinx
