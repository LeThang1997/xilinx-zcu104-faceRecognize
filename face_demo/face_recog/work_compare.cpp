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
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <array>
#include "work_compare.hpp"

using namespace std;
namespace xilinx {
namespace demo {

std::shared_ptr<Result> WorkCompare::process(std::shared_ptr<Result> input) {
  if (!input) {
    return nullptr;
  }
  auto ptr = dynamic_cast<FaceRecogResult *>(input.get());  
  if (!ptr->valid()) {
    return nullptr;
  }
  auto ret = lib_->compare(*(ptr->feature_.get()));
  if (std::get<0>(ret) == false) {
    LOG(INFO) << "Compare face: " << ptr->gid_ << " failed";
    return nullptr;
  } else {
    auto item = std::get<1>(ret);
    auto similarity = std::get<2>(ret);
    LOG(INFO) << "Compare face: " << ptr->gid_ << " successed"
              << ", similarity :" << similarity
              << ", name : " << item->name_
              << ", normal size : " << item->mat_.cols 
              << " * " << item->mat_.rows <<"!";
    input.reset(dynamic_cast<Result *>(new FaceCompareResult(*ptr, *item, similarity)));
   //printf("%s",item->name_);
  }
  
  return input;
}

}
}
