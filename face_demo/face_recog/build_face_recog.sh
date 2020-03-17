CXX=${CXX:-g++}
CPP_FILES="test_video_face_recog_multi.cpp 
						 demo_inner.cpp \
						 feature_lib.cpp \
						 work_detect.cpp \
						 work_track.cpp \
						 work_capture.cpp \
						 work_recog.cpp \
						 work_compare.cpp" 
$CXX -std=c++11 -I. -I/usr/include -L. -L/usr/lib -o test_video_face_recog_multi ${CPP_FILES} -lpthread -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -ldptracker -ldpfacedetect -ldpfacequality -ldpfacerecog -ldpfacelandmark -ldpfacefeature -ldpmath -lglog 
mv test_video_face_recog_multi ..

