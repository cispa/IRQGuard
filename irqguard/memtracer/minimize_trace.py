#! /usr/bin/env python3




def main():
    # read original config

    original_config = list()
    with open("prefetch_config.iguard", "r") as fd:
        for line in fd.readlines():
            original_config.append(int(line, 0))


    
    # strip cacheline offsets away 
    stripped_config = list()
    for addr in original_config:
        # zero out to get 64byte blocks 
        stripped_config.append(addr & ~0b111111)

    # remove duplicates
    minimized_config = list(set(stripped_config))
    
    # overwrite config with new values
    with open("prefetch_config.iguard", "w") as fd:
        for addr in minimized_config:
            fd.write(f"{addr:x}\n")

    



if __name__ == "__main__":
    main()
