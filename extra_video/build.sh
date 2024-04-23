#!/bin/bash

clang -g -o extra_video extra_video.c `pkg-config --cflags --libs  libavutil libavformat libavcodec`
