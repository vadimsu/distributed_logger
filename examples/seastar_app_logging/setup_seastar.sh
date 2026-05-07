#!/bin/bash
#cd examples/seastar_app_logging
if [ ! -d "seastar" ];then
	git clone https://github.com/scylladb/seastar.git
fi
cd seastar
git submodule update --init --recursive
./install-dependencies.sh
./configure.py --mode=release --without-demos --without-apps --without-tests
ninja -C build/release
ninja -C build/release install
