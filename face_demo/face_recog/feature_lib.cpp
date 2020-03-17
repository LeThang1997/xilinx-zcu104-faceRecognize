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
#include "feature_lib.hpp"

using namespace std;
namespace xilinx {
namespace demo {

static float feature_norm(const int8_t *feature) {
  int sum = 0;
  for (int i = 0; i < 512; ++i) {
      sum += feature[i] * feature[i];
  }
  return 1.f / sqrt(sum);
}

/// This function is used for computing dot product of two vector
static float feature_dot(const int8_t *f1, const int8_t *f2) {
   int dot = 0;
   for (int i = 0; i < 512; ++i) {
      dot += f1[i] * f2[i];
   }
   return (float)dot;
}

static float feature_compare(const int8_t *feature, const int8_t *feature_lib) {
                      
  float norm = feature_norm(feature);
  float feature_norm_lib = feature_norm(feature_lib);
  return feature_dot(feature, feature_lib) * norm * feature_norm_lib;
}

static float score_map(float score) { 
  return 1.0 / (1 + exp(-17.0836 * score + 5.5707)); 
}

static std::string rstrip(const std::string &s) {
  auto npos = s.rfind(' '); 
  if (npos != string::npos) {
    return s.substr(0, npos);
  } else {
    return s;
  } 
}

int FeatureLib::init() {
  ifstream ifile;
  ifile.open(file_name_);
  if (ifile.is_open()) {
    std::array<char, 2048> buf;
    while (ifile.getline(&buf[0], 2048)) {
      std::string s = &buf[0]; 
      auto npos = s.find(':', 0);
      std::string name = s.substr(0, npos);

     
      name = rstrip(name);
      LOG(INFO) << "Feature lib read name: " << name;
      int cnt = 0;
      std::vector<int8_t> feature(512);
      while (npos + 1< s.size()) {
        npos = npos + 1;
        auto begin = npos;
        npos = s.find(',', npos);
        if (npos == std::string::npos) {
          break;
        }
        feature[cnt] = std::stoi(s.substr(begin, npos));
        cnt++;
        if (cnt >= 512) {
          break;
        }
      }
      if (cnt < 512) {
        LOG(WARNING) << "Read feature error: " << name
                     << " size"  << cnt;
        continue;
      }
      face_lib_.emplace_back(name, feature);
      LOG(INFO) << "Read lib face : " << name;
    }
    
    ifile.close();
    loadImages();
    return 0;
  } else {
    LOG(ERROR) << "Cannot open face lib:" << file_name_;
    return -1;
  }
}

void FeatureLib::loadImages() {
  for (auto &f : face_lib_) 
  {

    LOG(INFO) << "Loading normal image: " << f.name_ << "...";  
    
      /*------------------------------------------------------------------------
      * Modifed : ThangLmb
      --------------------------------------------------------------------------*/
     // std::cout<< "Result: "<<  f.name_ << endl;

    auto image = cv::imread(f.name_);
    if (image.empty()) {
      LOG(ERROR) << "Cannot load normal image: " << f.name_;
    } else {
      LOG(INFO) << "Load normal image: " << f.name_ << " done"
                << " size: " << image.cols 
                << " * " << image.rows;
      f.mat_ = image;
    }
  } 
}

std::tuple<bool, const FaceItem *, float> FeatureLib::compare(const std::array<int8_t, 512> other) {
  FaceItem * p = nullptr;
  float score_original = 0.f;
  float score = 0.f;
  bool find = false;
  for (auto &i : face_lib_) {
    score_original = feature_compare(&other[0], &i.feature_[0]); 
    score = score_map(score_original); 
    if (score >=  similarity_threshold) {
      find = true;
      p = &i; 
      break;
    }
    score = 0.f;
  }
 
  return std::make_tuple(find, p, score); 
}

}
}
