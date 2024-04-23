#!/bin/bash

clang -g -o extra_audio extra_audio.c `pkg-config --cflags --libs  libavutil libavformat libavcodec`
