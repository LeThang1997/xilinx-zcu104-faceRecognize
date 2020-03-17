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
#include <string>
#include <xilinx/facedetect/facedetect.hpp>
#include <xilinx/facequality/facequality.hpp>
#include <xilinx/facerecog/facerecog.hpp>
#include <xilinx/tracker/tracker.hpp>
#include "demo_inner.hpp"
#include "work_thread.hpp"
#include "thread_intergrate.hpp"
#include "work_detect.hpp"
#include "work_track.hpp"
#include "work_capture.hpp"
#include "work_recog.hpp"
#include "work_compare.hpp"
#include "feature_lib.hpp"

using namespace std;
using namespace xilinx::demo;

int main(int argc, char* argv[]) {
  signal(SIGINT, MyThread::signal_handler);

  // -----Init Glog-----
  google::InitGoogleLogging(argv[0]);
  google::SetLogDestination(google::GLOG_INFO, "./");
  google::SetLogDestination(google::GLOG_WARNING, "./");
  google::SetLogDestination(google::GLOG_ERROR, "./");
  google::SetLogDestination(google::GLOG_FATAL, "./");
  //google::SetStderrLogging(google::GLOG_INFO);
  google::SetStderrLogging(google::GLOG_ERROR);
  FLAGS_colorlogtostderr = true;
  
  parse_opt(argc, argv);
  {
    auto channel_id = 0;

    // Init face feature lib
    const std::string file_name = "./face_feature.lib";
    auto face_lib = std::make_shared<FeatureLib>(file_name);
    if (face_lib->init() == -1) {
      LOG(ERROR) << "Face lib init error!";
      exit(-1);
    }

    printf("Face Recognition Xilinx -> ThangLMb <-\n");
    // Init decode thread
    auto decode_queue = std::shared_ptr<queue_t>{new queue_t{5}};
    auto decode_thread = std::unique_ptr<DecodeThread>(
        new DecodeThread{channel_id, g_avi_file[0], decode_queue.get()});

    // Init GUI thread
    auto gui_thread = GuiThread::instance();
    auto gui_queue = gui_thread->getQueue();
   
    // Init sorting thread
    auto sorting_queue =
        std::shared_ptr<queue_t>(new queue_t(5 * g_num_of_threads[0]));
    auto sorting_thread = std::unique_ptr<SortingThread>(
        new SortingThread(sorting_queue.get(), gui_queue, std::to_string(0)));

    // Init work threads and queues
    auto decode_queue_adapter = 
              std::unique_ptr<FrameQueue>(new FrameQueueAdapter(decode_queue, std::string{"decode"}));
    auto sorting_queue_adapter = 
              std::unique_ptr<FrameQueue>(new FrameQueueAdapter(sorting_queue, std::string{"sorting"}));

    auto frame_queue = std::make_shared<FrameQueue>(8, std::string{"frame"});
    auto compare_queue = std::make_shared<FrameQueue>(8, std::string{"compare"});
    auto recog_queue = std::make_shared<FrameQueue>(8, std::string{"recog"});

    auto dpu_thread = std::vector<std::unique_ptr<MyThread>>{};

    // Init tracker
    auto tracker = xilinx::tracker::Tracker::create(
        xilinx::tracker::Tracker::MODE_MULTIDETS|xilinx::tracker::Tracker::MODE_AUTOPATCH);

    for (int i = 0; i < g_num_of_threads[0]; ++i) {
      // Detect and track thread
      std::vector<std::unique_ptr<Work>> det_track_works;

      // WorkDetect to detect faces in a frame
      det_track_works.emplace_back(new WorkDetect(WorkDetect::createFilter(), tracker));

      // WorkTrack to get face id
      det_track_works.emplace_back(new WorkTrack(tracker));
      auto det_track_thread = std::unique_ptr<MyThread>(new WorkThread(
            std::move(det_track_works), 
            decode_queue_adapter.get(), // frame input queue
            frame_queue.get(), // frame output queue
            std::string("Detect_") + std::to_string(i), 
            recog_queue.get(), // queue for faces cropped in a frame
            true));
      dpu_thread.emplace_back(std::move(det_track_thread));
    }
    
    for (int i = 0; i < g_num_of_threads[0] * 2; ++i) {
      // Capture, recog and compare thread
      std::vector<std::unique_ptr<Work>> recog_works;

      // WorkCapture to get face quality, if quality lower than quality threshold, no need to recog
      recog_works.emplace_back(new WorkCapture(WorkCapture::createFilter()));  
      // WorkRecog to get face feature
      recog_works.emplace_back(new WorkRecog(WorkRecog::createFilter())); 
      // WorkCompare to compare face feature with feature library
      recog_works.emplace_back(new WorkCompare(face_lib));
      auto recog_thread = std::unique_ptr<MyThread>(new WorkThread(
            std::move(recog_works),
            recog_queue.get(), 
            compare_queue.get(), 
            std::string("Recog_") + std::to_string(i)));
            
      dpu_thread.emplace_back(std::move(recog_thread)); 
    }
    
    // Merge results 
    auto intergrate_thread = std::unique_ptr<ThreadIntergrate>(
                                      new ThreadIntergrate(frame_queue.get(), 
                                                           compare_queue.get(), 
                                                           sorting_queue_adapter.get(), 
                                                           std::string("intergrate"))); 
    dpu_thread.emplace_back(std::move(intergrate_thread)); 
    
    // Start everything
    MyThread::start_all();
    gui_thread->wait();
    MyThread::stop_all();
    MyThread::wait_all();
  }
  LOG(INFO) << "BYEBYE";
 
  return 0;
}


