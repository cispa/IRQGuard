#! /bin/bash
:wq
LD_PRELOAD="/home/dweber/self_compiled_openssl/libssl.so.1.0.0 /home/dweber/self_compiled_openssl/libcrypto.so    .1.0.0" sudo gdb -p $(cat /tmp/aes.pid)
