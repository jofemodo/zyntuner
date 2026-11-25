#!/bin/sh
# Build and run the DSP tests against bin/zyntuner.lv2 (run make first)
set -e
cd "$(dirname "$0")/.."
mkdir -p build
g++ -O2 -std=c++17 tests/test_dsp.cpp -o build/test_dsp -ldl
./build/test_dsp "$PWD/bin/zyntuner.lv2/zyntuner_dsp.so"
