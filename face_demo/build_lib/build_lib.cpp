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
#include <memory>
#include <utility>
#include <glog/logging.h>
#include <vector>
#include <array>
#include <string>
#include <cstdio>
#include <dirent.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <xilinx/facedetect/facedetect.hpp>
#include <xilinx/facequality/facequality.hpp>
#include <xilinx/facerecog/facerecog.hpp>
#include "../face_recog/demo_inner.hpp"
#include "../face_recog/data.hpp"

using namespace std;

constexpr float g_quality_threshold = 0.3;

vector<string> read_image_names(const string &dirname) {
  DIR *dir = NULL;
  struct dirent * ent= NULL;
  dir = opendir(dirname.c_str());
  if (nullptr == dir) {
    LOG(ERROR) << "Open directory : " << dirname << " error!";
  }
  vector<string> names;
  while (1) {
    ent = readdir(dir);
    if (ent != NULL) {
      if (ent->d_type == DT_REG) {
        //LOG(INFO) << ent->d_name << " is a normal file.";
        names.emplace_back(dirname + "/" + ent->d_name);
      } else {
        //LOG(INFO) << ent->d_name << " is not a normal file.";
      }
    } else {
      break;
    }
  }
  return names;
}

int main_for_jpeg_demo_face_full_single(int argc, char* argv[]) {
  if (argc <= 1) {
    std::cout << "usage : " << argv[0] << " image_directory" << endl;
    exit(1);
  }
  auto detect_model = xilinx::facedetect::FaceDetect::create(
                      xilinx::facedetect::DENSE_BOX_640x360);
  auto quality_model = xilinx::facequality::FaceQuality::create();
  auto recog_model = xilinx::facerecog::FaceRecog::create();

  // Load image names
  auto names = read_image_names(argv[1]);

  // Output file
  auto output_file = "face_feature.lib";
  ofstream out(output_file);

  for (auto &name : names) {
    auto image = cv::imread(name);
    if (image.empty()) {
      LOG(ERROR) << "Cannot load " << name;
      continue;
    }
    auto detect_result = detect_model->run(image);
    LOG(INFO) << "Image: " << name 
              << " size: " << image.cols << " * " << image.rows; 

    if (!detect_result.rects.empty()) {
      // Only use the first face
      auto r = detect_result.rects[0];
      cv::Rect_<int> bbox(r.x * image.cols, 
                          r.y * image.rows, 
                          r.width * image.cols, 
                          r.height * image.rows);
      auto face = image(bbox);
      auto quality_result = quality_model->run(face);

      if (quality_result.value > g_quality_threshold) { 
        auto rect_pair = xilinx::facerecog::FaceRecog::expand_and_align(image.cols, image.rows, 
                r.x * image.cols, r.y * image.rows, r.width * image.cols, r.height * image.rows,
                0.2, 0.2, 8, 8);
        auto recog_result = recog_model->run_fixed(image(rect_pair.first), 
                              rect_pair.second.x, rect_pair.second.y,
                              rect_pair.second.width, rect_pair.second.height);
        
        out << name << " : "; 
        for (auto n : *(recog_result.feature)) {
          out << (int)n << ",";
        }
        out << endl;
        LOG(INFO) << name << " : generated features";
      } else {
        LOG(ERROR) << name << " quality too low " << quality_result.value;
      }
    } else {
      LOG(WARNING) << "Image: " << name << " has no face!"; 
    }
  }

  // Write to file
  out.close();
  LOG(INFO) << "Features genereated in: " << output_file;
  return 0;
}

int main(int argc, char *argv[]) {
  return main_for_jpeg_demo_face_full_single(argc, argv);
}

