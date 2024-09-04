#! /bin/bash

# we need to allow arbitrary process injections for this to work
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope

cd /home/artifact/irqguard/memtracer
sudo mkdir /var/empty | true
echo "[+] Starting OpenSSH..."
sudo LD_PRELOAD="/home/artifact/case-study-openssh/self_compiled_openssl/libssl.so.1.0.0 /home/artifact/case-study-openssh/self_compiled_openssl/libcrypto.so.1.0.0" ./iguard_create_memtrace.sh /home/artifact/case-study-openssh/openssh-7-5/openssh-portable-V_7_5_P1/sshd -p 31337 -ddde

