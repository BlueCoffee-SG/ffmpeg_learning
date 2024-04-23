#!/bin/bash

clang -g -o remux remux.c `pkg-config --cflags --libs  libavutil libavformat libavcodec`
