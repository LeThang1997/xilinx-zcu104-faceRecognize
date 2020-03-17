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
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>
#include <thread>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "frame_queue.hpp"
#include "work.hpp"
#include <iostream>
namespace xilinx {
namespace demo {

struct ThreadIntergrate: public MyThread {
  ThreadIntergrate(FrameQueue* queue_image,
                   FrameQueue* queue_face,
                   FrameQueue* queue_out, const std::string& suffix)
      : MyThread{},
        queue_image_{queue_image},
        queue_face_{queue_face},
        queue_out_{queue_out},
        suffix_{suffix} {}
  virtual ~ThreadIntergrate() {}
  virtual int run() override {
    std::shared_ptr<Result> frame_in;
    if (!queue_image_->pop(frame_in, std::chrono::milliseconds(10))) {
      return 0;
    }
    LOG(INFO) << "[" << name() << "] "
              << "queue: " << queue_image_->info()
              << " output frame :" << (dynamic_cast<FrameResult *>(frame_in.get()))->frame_info_.frame_id;

    std::shared_ptr<Result> face;
    if (queue_face_->pop(face, std::chrono::milliseconds(1))) {
      if (face) {
        LOG(INFO) << "[" << name() << "] "
                  << "queue: " << queue_face_->info()
                  << " output face:" << (dynamic_cast<FaceCompareResult *>(face.get()))->gid_;
        setMatchedFace(dynamic_cast<FaceCompareResult *>(face.get())); 
      }
    } 

    show(frame_in);
    while (!queue_out_->push(frame_in, std::chrono::milliseconds(10))) {
 
      LOG(INFO) << name() 
                << " Try push queue " << queue_out_->info() 
                << " again " << queue_out_->capacity() << " vs " << queue_out_->size();
      if (is_stopped()) {
        return -1;
      }
    }
    return 0;
  }

  void show(std::shared_ptr<Result> frame) 
  {
    showDetect(frame);
    if (face_) 
    {
      showCompare(frame);
      //printf("This is frame face recognition \n");
    
    }
  }
  void showDetect(std::shared_ptr<Result> frame) {
    auto frame_result = dynamic_cast<FrameResult *>(frame.get());

    // Draw face rectangles
    for (auto &r : frame_result->results_) {
      auto f = dynamic_cast<FaceDetTrackResult *>(r.get());
      if (!f->valid()) {
        continue;
      }
      cv::rectangle(f->full_image_, cv::Rect{cv::Point(f->bbox_.x,  f->bbox_.y),
                                  cv::Size{(int)(f->bbox_.width),
                                           (int)(f->bbox_.height)}},
                  0xff);
      cv::Scalar color{0, 255, 255};

      // Show tracked id
      cv::putText(f->full_image_, std::to_string(f->gid_), 
                cv::Point{(int)(f->bbox_.x + 10), (int)(f->bbox_.y + 10)}, 
                cv::FONT_HERSHEY_PLAIN, 1, color, 1, 1, 0);
      /*------------------------------------------------------------------------
      * Modifed : ThangLmb
      --------------------------------------------------------------------------*/
     // cv::imwrite("Ressult.jpg",f->full_image_)  ;
    }
 
  }

  void setMatchedFace(const FaceCompareResult * f) 
  {
     LOG(INFO) << name() 
               << " set matched face: " << f->gid_;
     face_.reset(new FaceCompareResult);

     cv::resize(f->image_, face_->image_, cv::Size{96, 112}); 
     face_->name_ = f->name_;
     face_->gid_ = f->gid_;
     face_->similarity_ = f->similarity_;
     face_->quality_ = f->quality_;
     face_->gender_ = f->gender_;
     face_->age_ = f->age_;
     /*------------------------------------------------------------------------
      * Modifed : ThangLmb
      --------------------------------------------------------------------------*/
      char id_result[8];
      int k=0;
      char name_result[f->name_.size() + 1];
	    strcpy(name_result, f->name_.c_str());
      ///std::cout<<"Ressult: "<<name_result<<std::endl  ;
      for(int i =0;i<sizeof(name_result);i++)
      {
          if(name_result[i]=='/')
          {
            for(int j=i+1 ;j<i+9;j++)
            {
              id_result[k]=name_result[j];
              k++;
              //if(name_result[j]=='.')
              //break;
            }
          }
      }
      std::cout<<"Ressult: "<<id_result<<std::endl  ;
     //printf("result : %s\n", id_result);
      printf("int : %d\n", k);

    ///////////////////////////////////////////////////////////////
    const char* server_name  = "192.168.1.1";
   int server_port = 9999; // FIX ME

	int sock;
   struct sockaddr_in server_address;
   memset(&server_address, 0, sizeof(server_address));
   server_address.sin_family = AF_INET;

   // creates binary representation of server name
   // and stores it as sin_addr
   inet_pton(AF_INET, server_name, &server_address.sin_addr);

   // htons: port in network order format
   server_address.sin_port = htons(server_port);

   if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
   {
      printf("Could not connect Server\n");
   }
   // TCP is connection oriented, a reliable connection
   // *must* be established before any data is exchanged
   if (::connect(sock, (struct sockaddr*) &server_address,sizeof(server_address)) < 0)
   {
       printf("Could not connect Server\n");
   }
	/// send to server
	::send(sock, id_result, 9, 0);

  } 

  void showCompare(std::shared_ptr<Result> frame) {
    auto frame_result = dynamic_cast<FrameResult *>(frame.get());
    auto &full_image = frame_result->frame_info_.mat;
    auto frame_width = full_image.cols;
    auto frame_height = full_image.rows;
   
    // Draw normal face
    LOG(INFO) << name() 
              << " Show face :" << face_->gid_ << ", size :" << frame_width
              << " * " << frame_height;
    cv::Rect_<int> rect{0, frame_height - 112, 96, 112};
    face_->image_.copyTo(full_image(rect));

    cv::Scalar color{255, 0, 0};

    // Show face quality
    //cv::putText(full_image, std::to_string(face_->quality_), 
    //          cv::Point{0, frame_height - 112}, 
    //          cv::FONT_HERSHEY_PLAIN, 0.5, color, 1, 1, 0);
    // Show face name 
    cv::putText(full_image, face_->name_, 
              cv::Point{0, frame_height - 112}, 
              cv::FONT_HERSHEY_PLAIN, 1, color, 1, 1, 0);
    // Show face similarity 
    int similar = face_->similarity_ * 100;
    cv::putText(full_image, std::string("similarity:") + std::to_string(similar), 
              cv::Point{0, frame_height - 112 + 20}, 
              cv::FONT_HERSHEY_PLAIN, 1, color, 1, 1, 0);

  } 

  virtual std::string name() override { return std::string("T-Intergrate-") + suffix_; }
private:
  std::vector<std::unique_ptr<Work>> works_;
  std::shared_ptr<FaceCompareResult> face_;
  FrameQueue* queue_image_;
  FrameQueue* queue_face_;
  FrameQueue* queue_out_;
  std::string suffix_;
};

} // end of demo
} // end of xilinx
