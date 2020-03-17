#!/bin/sh
set -e
CURRENT_DIR="$( cd "$( dirname "$0" )" && pwd )"
cd ${CURRENT_DIR}/build_lib && ./build_lib.sh
cd ${CURRENT_DIR}/face_recog && ./build_face_recog.sh
