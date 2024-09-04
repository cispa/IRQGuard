## Instructions 

### Setup custom OpenSSL and OpenSSH server
To make this case study work, we first need to install the OpenSSH server with the protected OpenSSL library.
First, you need to correct the paths for the `pmc_config.iguard` file in `self_compiled_openssl/crypto/aes/aes_core.c:839`.
Be careful, as you need to set this to an *absolute* path.

After copying the `self_compiled_openssl` folder to your system, you need to build it by following these steps.
First, create the Makefile using `./config shared no-asm`.
Next, add the flag `-pthread` to the `CFLAG` variable of the Makefile (the first position is ok).
Eventually, compile the library by running `make` (not `make -j`).

Now we need to configure OpenSSH.
For this, copy the folder `openssh-7-5` to your system and correct the paths in `openssh-7-5/run.sh` to point to the `self_compiled_openssl` folder. 


### Test the Setup
Next, load the IRQGuard kernel module.
If the previous steps all succeeded, you can now start the OpenSSH server by running the file `./run.sh` inside the OpenSSH directory.
You can test the connection by connecting to the server and enabling the (now protected) ciphersuite, e.g., `ssh -o "Ciphers=aes128-ctr" localhost -p 31337`.
Note that the terminal calling `./run.sh` must remain open for the connection to work.


### Lowering the threshold
The base setup does not use the prefetching feature of IRQGuard.
To generate a prefetching profile, run the script `./run-prefetch-generation.sh` and connect to the server as described above.
Afterwards, the prefetch configuration should exist as `/prefetch_config.iguard` (yes, indeed, in the root directory of the system).
Eventually, copy the prefetch config to `case-study-openssh`.
If it works, the invocation of `run.sh` should no longer print the warning message that prefetching is disabled due to a missing configuration file.