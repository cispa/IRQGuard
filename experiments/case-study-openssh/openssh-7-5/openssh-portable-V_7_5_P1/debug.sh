#! /bin/bash

sudo mkdir /var/empty | true
echo "[+] Starting OpenSSH..."
#sudo LD_PRELOAD="/home/dweber/self_compiled_openssl/libssl.so.1.0.0 /home/dweber/self_compiled_openssl/libcrypto.so.1.0.0" $PWD/sshd -p 31337 -ddde -f $PWD/sshd_config
sudo LD_PRELOAD="/home/dweber/self_compiled_openssl/libssl.so.1.0.0 /home/dweber/self_compiled_openssl/libcrypto.so.1.0.0" gdb --args $PWD/sshd -p 31337 -ddde
#echo "[+] OpenSSH started."
#echo "[+] Supported Ciphers:"
#sudo LD_PRELOAD="/home/dweber/self_compiled_openssl/libssl.so.1.0.0 /home/dweber/self_compiled_openssl/libcrypto.so.1.0.0" $PWD/ssh -Q cipher
#sudo LD_PRELOAD="/home/dweber/self_compiled_openssl/libssl.so.1.0.0 /home/dweber/self_compiled_openssl/libcrypto.so.1.0.0" ldd sshd

