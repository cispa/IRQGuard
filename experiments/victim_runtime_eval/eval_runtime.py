#! /usr/bin/env python3


import subprocess
import statistics
import time
import re

#NO_ITERATIONS = 2500
NO_ITERATIONS = 10
VICTIM_BINARY = "./victim"
VICTIM_SOURCE = "./victim.c"
ITERATION_CLASSES = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]

AGGREGATE_DATA = False

def execute_cmd(s):
    return subprocess.check_output(s,
                                   shell=True, 
                                   stderr = subprocess.STDOUT).decode()


def eval_with_iguard(iterations):
    # compile
    execute_cmd("make clean")
    execute_cmd("make iguard_without_attack")

    realtime_re = re.compile(r"TS:([0-9].+)")
    times = list()
    for i in range(NO_ITERATIONS):
        print(f"{iterations} -> {i}/{NO_ITERATIONS}")
        # %e is the formatter for realtime (only works in sh; not in bash or zsh)
        #output = execute_cmd(f'time -f "TS:%e" {VICTIM_BINARY}')
        before = time.time()
        output = execute_cmd(f"{VICTIM_BINARY} {iterations}")
        after = time.time()
        #realtime_unparsed = realtime_re.findall(output)
        #time_seconds = float(realtime_unparsed[0])
        ts = after - before

        print(ts)
        times.append(ts)
    return times

def eval_without_iguard(iterations):
    # compile
    execute_cmd("make clean")
    execute_cmd("make")

    realtime_re = re.compile(r"TS:([0-9].+)")
    times = list()
    for i in range(NO_ITERATIONS):
        print(f"{iterations} -> {i}/{NO_ITERATIONS}")
        # %e is the formatter for realtime (only works in sh; not in bash or zsh)
        #output = execute_cmd(f'time -f "TS:%e" {VICTIM_BINARY}')
        before = time.time()
        output = execute_cmd(f"{VICTIM_BINARY} {iterations}")
        after = time.time()
        ts = after - before
        #realtime_unparsed = realtime_re.findall(output)
        #print(realtime_unparsed)
        #time_seconds = float(realtime_unparsed[0])
        print(ts)
        times.append(ts)
    return times

def check_source_constraints():
    with open(VICTIM_SOURCE, "r") as fd:
        line_no = 1
        for line in fd:
            if line.startswith("#define") and "iguard_ENABLE_PROTECTION" in line:
                print(f"[ERR] Looks like iguard_ENABLE_PROTECTION is defined "
                      f"(line {line_no}). This will break the eval, so uncomment it!")
                exit(1)
            if line.startswith("#define") and "ENABLE_ATTACK" in line:
                print(f"[ERR] Looks like ENABLE_ATTACK is defined "
                      f"(line {line_no}). This will break the eval, so uncomment it!")
                exit(1)
            line_no += 1
    

def main():
    check_source_constraints()
    
    try:
        execute_cmd("sudo rmmod iguardV3/iguard.ko")
    except:
        pass

    runtimes_iguard = list()
    runtimes_no_iguard = list()
    fd_iguard = open("runtime_with_iguard.csv", "w")
    fd_wo_iguard = open("runtime_without_iguard.csv", "w")

    fd_wo_iguard.write("iterations;runtimeSeconds\n")
    for iteration_number in ITERATION_CLASSES:

        without_iguard_runtimes = eval_without_iguard(iteration_number)
        if AGGREGATE_DATA:
            runtime = statistics.median(without_iguard_runtimes)
            fd_wo_iguard.write(f"{iteration_number};{runtime}\n")
        else:
            for runtime in without_iguard_runtimes:
                fd_wo_iguard.write(f"{iteration_number};{runtime}\n")
                runtimes_iguard.append(runtime)

    fd_iguard.write("iterations;runtimeSeconds\n")
    for iteration_number in ITERATION_CLASSES:
        execute_cmd("sudo insmod iguardV3/iguard.ko")
        with_iguard_runtimes = eval_with_iguard(iteration_number)
        execute_cmd("sudo rmmod iguardV3/iguard.ko")
        if AGGREGATE_DATA:
            runtime = statistics.median(with_iguard_runtimes)
            fd_iguard.write(f"{iteration_number};{runtime}\n")
        else: 
            for runtime in with_iguard_runtimes:
                fd_iguard.write(f"{iteration_number};{runtime}\n")
                runtimes_no_iguard.append(runtime)
        
    fd_iguard.close()
    fd_wo_iguard.close()

    



if __name__ == "__main__":
    main()
