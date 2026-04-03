#!/bin/bash
cd examples/seastar_app_logging
if [ !-d "seastar" ];then
	git clone https://github.com/scylladb/seastar.git
fi
cd seastar
git submodule update --init --recursive
sudo ./install-dependencies.sh
./configure.py --mode=release --without-demos --without-apps --without-tests
ninja -C build/release
sudo ninja -C build/release install
