#! /usr/bin/env python3

import sys

SECRET = "01101001"

def find_starting_offset(line):
    for i in range(8):
        if line.startswith:
            pass

def string_rotate(s,d): 
    Lfirst = s[0 : d] 
    Lsecond = s[d :] 
    Rfirst = s[0 : len(s)-d] 
    Rsecond = s[len(s)-d :] 
    # left rotate
    return Lsecond + Lfirst

    
def main():
    for line in sys.stdin:
        if line.startswith("0") or line.startswith("1"):
            secret_leak = line.strip()
    print(f"Secret Leak: {secret_leak}")

    # ignore first 8 bytes (likely noise)
    #secret_leak = secret_leak[8:]

    initial_offset = -1
    best_start = len(secret_leak) + 1
    for secret_offset in range(8):
        s = string_rotate(SECRET, secret_offset)
        idx = secret_leak.index(s)
        if idx < best_start:
            initial_offset = secret_offset
            best_start = idx
    assert initial_offset != -1
    assert best_start != len(secret_leak) + 1

    corr = 0
    err = 0
    for i in range(len(secret_leak) - best_start):
        c = secret_leak[best_start + i]
        #print(f"c ({c}) == SECRET({SECRET[((initial_offset + i) % 8)]})")
        if c == SECRET[(initial_offset + i) % 8]:
            corr += 1
        else:
            err += 1
    
    print(f"\033[92mCorrect: {corr}\033[0m | \033[91mFalse: {err}\033[0m")
    print(f"\033[93mSkipped first {best_start} bytes\033[0m")
    print(f"\033[93mInitial Offset {initial_offset} bytes\033[0m")
            
    
    
    
    
    
        
    



if __name__ == "__main__":
    main()
