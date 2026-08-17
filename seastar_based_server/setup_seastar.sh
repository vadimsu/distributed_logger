#!/bin/bash
#cd examples/seastar_app_logging
if [ ! -d "seastar" ];then
	git clone https://github.com/scylladb/seastar.git
fi
cd seastar
git checkout 26badcb14c1b8a03547d5b7fb24ab6b5f4c42299
git submodule update --init --recursive
echo "Installing Seastar dependencies"
./install-dependencies.sh
echo "Configuring Seastar"
./configure.py --mode=release --without-demos --without-apps --without-tests
echo "Building Seastar"
ninja -C build/release
echo "Installing Seastar"
ninja -C build/release install
