#!/bin/bash
cd mongo-cxx-driver/build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 ..
cd ..
make
cd ..
make
