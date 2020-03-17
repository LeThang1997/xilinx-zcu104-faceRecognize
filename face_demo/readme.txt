Face_demo 
  The face demo can detect faces from video or camera with input size 640*360, extract face feature and compare with feature lib. 
   You can run this demo as following steps.
   1) Build feature lib: run executale file with parameter of "image_directory", for example:
      ./build_feature_lib images
   2) Run face recognition demo 
   2.1) If you use video file(.mp4), for example:
      ./test_video_face_recog_multi colleage.mp4 
      You can use Ctrl-C to stop the progress.
   2.2) If you use camera, run with parameter of channel number, now only support one channel
      ./test_video_face_recog_multi 0 
   Running on ZCU102, the FPS depends on how many faces in a frame. If not more than 10 faces, the FPS usually reach 30. 
  
    You can build the program by running build.sh
    ./build.sh 
