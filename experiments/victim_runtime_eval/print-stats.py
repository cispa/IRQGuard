#! /usr/bin/env python3

import statistics

def main():
    runtimes_with_iguard = dict()
    runtimes_without_iguard = dict()

    with open("runtime_with_iguard.csv", "r") as fd:
        for line in fd.readlines()[1:]:
            iteration, runtime = line.split(";")
            iteration = int(iteration)
            runtime = float(runtime)
            if iteration in runtimes_with_iguard:
                runtimes_with_iguard[iteration].append(runtime)
            else:
                runtimes_with_iguard[iteration] = [runtime]

    with open("runtime_without_iguard.csv", "r") as fd:
        for line in fd.readlines()[1:]:
            iteration, runtime = line.split(";")
            iteration = int(iteration)
            runtime = float(runtime)
            if iteration in runtimes_without_iguard:
                runtimes_without_iguard[iteration].append(runtime)
            else:
                runtimes_without_iguard[iteration] = [runtime]

    assert runtimes_with_iguard.keys() == runtimes_without_iguard.keys()

    print("iterations\t&\toverhead\t&\truntime-no-iguard (median) \t&\truntime-iguard (median)")
    for iteration in runtimes_with_iguard.keys():
        overheads = list()
        for rt_without_iguard, rt_with_iguard in zip(runtimes_without_iguard[iteration], runtimes_with_iguard[iteration]):
            curr_overhead = (rt_with_iguard / rt_without_iguard) * 100 - 100
            overheads.append(curr_overhead)
        median_overhead = statistics.median(overheads)
        std_overhead = statistics.stdev(overheads)
        median_without_iguard = statistics.median(runtimes_without_iguard[iteration]) * 1000  # * 1000 -> ms to sec
        median_with_iguard = statistics.median(runtimes_with_iguard[iteration]) * 1000  # * 1000 -> ms to sec
        print(f"{iteration}\t&\t\\SI{{{median_overhead:.2f}}}{{\\percent}}($\pm$ \SIx{{{std_overhead:.2f}}})\t&\t\\SI{{{median_without_iguard:.2f}}}{{\\milli\\second}}\t&\t\\SI{{{median_with_iguard:.2f}}}{{\\milli\\second}} \\\\")


            





if __name__ == "__main__":
    main()
