#! /usr/bin/env python3

import subprocess
import statistics

NO_ITERATIONS = 10
VICTIM_BINARY = "./victim"
VICTIM_SOURCE = "./victim.c"


# DO NOT CHANGE OR THINGS WILL BREAK
SECRET = "01101001"
BITS_TO_ATTACK = 1000


def execute_cmd(s):
    return subprocess.check_output(s, shell=True, timeout=10).decode()

    
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

    
def check_correct_attack_bits(atk_output):
    for line in atk_output.split("\n"):
        if line.startswith("0") or line.startswith("1"):
            secret_leak = line.strip()

    initial_offset = -1
    best_start = len(secret_leak) + 1
    for secret_offset in range(8):
        s = string_rotate(SECRET, secret_offset)
        try: 
            idx = secret_leak.index(s)
        except ValueError:
            return 0,BITS_TO_ATTACK,0
        if idx < best_start:
            initial_offset = secret_offset
            best_start = idx
    assert initial_offset != -1
    assert best_start != len(secret_leak) + 1

    corr = 0
    err = 0
    for i in range(len(secret_leak) - best_start):
        c = secret_leak[best_start + i]
        if c == SECRET[(initial_offset + i) % 8]:
            corr += 1
        else:
            err += 1
    
    #print(f"\033[92mCorrect: {corr}\033[0m | \033[91mFalse: {err}\033[0m")
    #print(f"\033[93mSkipped first {best_start} bytes\033[0m")
    return corr, err, best_start
    
    



def eval_attack_baseline():
    # compile
    execute_cmd("make attack_baseline")

    # execute
    iterations_left = NO_ITERATIONS
    results = list()
    while iterations_left > 0:
        print(f"iterations left: {iterations_left}")
        try:
            output = execute_cmd(f"sudo {VICTIM_BINARY}")
            iterations_left -= 1
            correct_bits, wrong_bits, best_start = check_correct_attack_bits(output)
            #assert correct_bits + wrong_bits == BITS_TO_ATTACK
            correct_percentage = (correct_bits / (correct_bits + wrong_bits)) * 100
            results.append(correct_percentage)
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
            print("Subprocess Error. Skipping.")
            continue
    
    return results


def eval_iguard_no_attack():
    execute_cmd("make iguard_without_attack")
    try:
        execute_cmd("sudo insmod iguardV3/iguard.ko")
    except subprocess.CalledProcessError:
        print("[!] Skipping KModule Insertion. (Can be ignored if the module is already loaded)")
    pass

    iterations_left = NO_ITERATIONS
    results = list()
    detected = 0
    undetected = 0
    while iterations_left > 0:
        print(f"iterations left: {iterations_left}")
        try:
            output = execute_cmd(f"sudo {VICTIM_BINARY}")
            iterations_left -= 1
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
            print("Subprocess Error. Skipping.")
            continue
        print(output)
        use_next_result = False
        for line in output.split("\n"):
            if "[!!]Attack discovered after" in line:
                detection_idx = int(line.split()[-2])
                results.append(detection_idx)
                detected += 1
                break
        else:
            undetected += 1
        
    assert detected + undetected == NO_ITERATIONS
    return detected, undetected, results

def eval_iguard_attack():
    execute_cmd("make iguard_with_attack")
    try:
        execute_cmd("sudo insmod iguardV3/iguard.ko")
    except subprocess.CalledProcessError:
        print("[!] Skipping KModule Insertion. (Can be ignored if the module is already loaded)")
    pass

    iterations_left = NO_ITERATIONS
    results = list()
    detected = 0
    undetected = 0
    while iterations_left > 0:
        print(f"iterations left: {iterations_left}")
        try:
            output = execute_cmd(f"sudo {VICTIM_BINARY}")
            iterations_left -= 1
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
            print("Subprocess Error. Skipping.")
            continue
        print(output)
        use_next_result = False
        for line in output.split("\n"):
            if "[!!]Attack discovered after" in line:
                detection_idx = int(line.split()[-2])
                results.append(detection_idx)
                detected += 1
                break
        else:
            undetected += 1
        
    assert detected + undetected == NO_ITERATIONS
    return detected, undetected, results

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

    correct_percentages = eval_attack_baseline()
    atk_detected, atk_undetected, atk_detection_indices = eval_iguard_attack()
    noatk_detected, noatk_undetected, noatk_detection_indices = eval_iguard_no_attack()

    #
    # BASELINE
    #
    print(f"Number Iterations: {NO_ITERATIONS}")
    print("=" * 20)
    print("=== Attack Baseline (Correct Percentages Leaked) ===")
    print(f"median:\t\t{statistics.median(correct_percentages)}%")
    print(f"avg:\t\t{sum(correct_percentages)/len(correct_percentages)}%")
    print(f"min:\t\t{min(correct_percentages)}%")
    print(f"max:\t\t{max(correct_percentages)}%")
    print("=" * 20)

    #
    # WITH ATTACK
    #
    print("=" * 20)
    print("=== iguard *WITH* Attack (Attack Detection Index) ===")
    print(f"median:\t\t{statistics.median(atk_detection_indices)}")
    print(f"avg:\t\t{sum(atk_detection_indices)/len(atk_detection_indices)}")
    print(f"std:\t\t{statistics.stdev(atk_detection_indices)}")
    print(f"min:\t\t{min(atk_detection_indices)}")
    print(f"max:\t\t{max(atk_detection_indices)}")
    print(f"Detected: {atk_detected}")
    print(f"Undetected: {atk_undetected}")
    print("=" * 20)

    #
    # WITHOUT ATTACK
    #
    print("=" * 20)
    print("=== iguard *WITHOUT* Attack (Attack Detection Index) ===")
    #print(f"median:\t\t{statistics.median(noatk_detection_indices)}")
    #print(f"avg:\t\t{sum(noatk_detection_indices)/len(noatk_detection_indices)}")
    #print(f"min:\t\t{min(noatk_detection_indices)}")
    #print(f"max:\t\t{max(noatk_detection_indices)}")
    print(f"Detected: {noatk_detected}")
    print(f"Undetected: {noatk_undetected}")
    print("=" * 20)

    
    #
    # F-Score
    #
    true_pos = atk_detected
    false_pos = noatk_detected
    false_neg = atk_undetected

    recall = true_pos / (true_pos + false_neg)
    precision = true_pos / (true_pos + false_pos)
    f_score = 2 * ((precision * recall) / (precision + recall))

    print("Recall:   ", recall)
    print("Precision:", precision)
    print("F-score:  ", f_score)





    


    



if __name__ == "__main__":
    main()
