#!/bin/sh
clang dynamicprefixvici.c /usr/local/lib/ipsec/libvici.so /usr/lib/libm.so \
    -I/usr/local/include -I/usr/lib/include \
    -Ofast -march=native -odynamicprefixvici