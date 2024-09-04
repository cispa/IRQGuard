#! /bin/bash

sudo mkdir /var/empty | true
echo "[+] Starting OpenSSH..."

#while true;
#do
	sudo LD_PRELOAD="/home/artifact/case-study-openssh/self_compiled_openssl/libssl.so.1.0.0 /home/artifact/case-study-openssh/self_compiled_openssl/libcrypto.so.1.0.0" $PWD/sshd -p 31337 -ddde;
#done

