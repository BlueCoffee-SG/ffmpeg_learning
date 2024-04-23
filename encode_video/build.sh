#!/bin/bash

clang -g -o encode_video encode_video.c `pkg-config --cflags --libs  libavutil libavformat libavcodec`


