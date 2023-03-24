build:
	clang dynamicprefixvici.c /usr/local/lib/ipsec/libvici.so -I/usr/local/include -Ofast -march=native -odynamicprefixvici

install:
	install -g wheel -o root -s dynamicprefixvici /usr/local/sbin