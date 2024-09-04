#! /bin/bash

sudo mkdir /var/empty | true
echo "[+] Starting client..."
sudo LD_PRELOAD="/home/dweber/self_compiled_openssl/libssl.so.1.0.0 /home/dweber/self_compiled_openssl/libcrypto.so.1.0.0" $PWD/ssh -vvv -o"Ciphers=aes128-gcm@openssh.com" \
	-p 31337 dweber@127.0.0.1 -i /home/dweber/ssh_test

