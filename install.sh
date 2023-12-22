#!/bin/sh

gcc `pkg-config --cflags glib-2.0` -shared -o libankiconnect.so ankiconnect.c `pkg-config --libs glib-2.0` -fPIC || exit

sudo cp ./ankiconnect.h /usr/local/include/
sudo cp ./libankiconnect.so /usr/lib
