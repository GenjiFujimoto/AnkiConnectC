#!/bin/sh

gcc `pkg-config --cflags glib-2.0` -shared -o libankiconnectc.so ankiconnectc.c `pkg-config --libs glib-2.0` -fPIC || exit

sudo cp ./ankiconnectc.h /usr/local/include/
sudo cp ./libankiconnectc.so /usr/lib
