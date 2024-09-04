#! /bin/bash
export LD_LIBRARY_PATH=/home/dweber/self_compiled_openssl
./configure --prefix=/opt CFLAGS="-UUSE_BUILTIN_RIJNDAEL -I/home/dweber/self_compiled_openssl/include" \
	--with-ldflags="-L/home/dweber/self_compiled_openssl" \
	--with-openssl="/home/dweber/self_compiled_openssl" \
	--without-seccomp
make -j
