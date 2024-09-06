#! /usr/bin/env python3


# TODO: change back to /
FNAME = "./prefetch_config.iguard"

def main():
    lines_to_keep = list()

    # 12 hex chars (48bit addrs) + "0x" + "\n"
    max_len = 12 + 2 + 1
    with open(FNAME, "r") as fd:
        for line in fd.readlines():
            if line.startswith("0x") and len(line) <= max_len:
                # seems valid
                lines_to_keep.append(line)

    with open(FNAME, "w") as fd:
        for line in lines_to_keep:
            fd.write(line)
    

            
    



if __name__ == "__main__":
    main()
