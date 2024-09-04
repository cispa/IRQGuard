#! /bin/bash
export LD_LIBRARY_PATH=/home/dweber/self_compiled_openssl
export LD_PRELOAD="/home/dweber/self_compiled_openssl/libssl.so.1.0.0 /home/dweber/self_compiled_openssl/libcrypto.so.1.0.0"
#./configure --prefix=/opt CFLAGS="-I/home/dweber/self_compiled_openssl/include" \
	--with-ldflags="-L/home/dweber/self_compiled_openssl"
#sudo make 
sudo make install
