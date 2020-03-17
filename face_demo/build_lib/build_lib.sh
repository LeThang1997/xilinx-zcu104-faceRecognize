CXX=${CXX:-g++}
CPP_FILES="build_lib.cpp"
$CXX -std=c++11 -I. -L. -o build_feature_lib ${CPP_FILES} -lpthread -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -ldpfacedetect -ldpfacequality -ldpfacerecog -ldpfacelandmark -ldpfacefeature -ldpmath -lglog 
mv build_feature_lib .. 

