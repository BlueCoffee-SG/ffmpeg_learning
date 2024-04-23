  #!/bin/bash

clang -g -o cut cut.c `pkg-config --cflags --libs  libavutil libavformat libavcodec`
