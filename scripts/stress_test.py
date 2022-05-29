#!/usr/bin/env python3

import time
import os
import filecmp
import enum
from tokenize import String

print("TEST SCRIPT")

if not os.path.isdir("tests_out"):
    os.makedirs("tests_out")
    print("creating tests output folder tests_out/")
else:
    print("folder tests_out exists")

#stress tests
stress1= "bin/sdstore proc-file ./teste.txt ./tests_out/stress1_out encrypt gcompress nop encrypt bcompress nop encrypt nop gcompress encrypt"
stress2= "bin/sdstore proc-file ./teste.txt ./tests_out/stress2_out encrypt gcompress gcompress gcompress encrypt nop bcompress encrypt nop bcompress encrypt"
stress3= "bin/sdstore proc-file ./teste.txt ./tests_out/stress3_out gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress gcompress"
stress4= "bin/sdstore proc-file ./teste.txt ./tests_out/stress4_out nop nop nop nop nop nop nop nop nop"

#unstress
unstress1= "bin/sdstore proc-file ./tests_out/stress1_out ./tests_out/unstress1_out.txt decrypt gdecompress decrypt bdecompress decrypt gdecompress decrypt"
unstress2= "bin/sdstore proc-file ./tests_out/stress2_out ./tests_out/unstress2_out.txt decrypt bdecompress decrypt bdecompress decrypt gdecompress gdecompress gdecompress decrypt"
unstress3= "bin/sdstore proc-file ./tests_out/stress3_out ./tests_out/unstress3_out.txt gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress gdecompress"
unstress4= "bin/sdstore proc-file ./tests_out/stress4_out ./tests_out/unstress4_out.txt nop"

stress_tests = [stress1,stress2,stress3,stress4]
unstress_tests = [unstress1,unstress2,unstress3,unstress4]

total_tests = len(stress_tests)
passed_tests = 0

#run stress
print("STRESS")
i = 0
while(i<total_tests):
    print("[%d/%d] testing %s" % (i+1, total_tests, stress_tests[i]))
    start_time1 = time.time()
    os.system(stress_tests[i])
    end_time = time.time()
    execution_time = end_time - start_time1
    print("execution time: %s" % execution_time)
    #os.system(expected[i])
    i += 1

#run stress results
print("TESTING STRESS")
i = 0
while(i<total_tests):
    print("[%d/%d] testing revert %s" % (i+1, total_tests, unstress_tests[i]))
    os.system(unstress_tests[i])
    if (filecmp.cmp("tests_out/unstress" + str(i+1) + "_out.txt", "teste.txt", shallow=True)):
        passed_tests += 1
    i += 1


print("[%d/%d] %d/%d test passed" % (i, total_tests, passed_tests, total_tests))

print("entering loop")

n = 0
i = 0
while(n<50):
    while(i<total_tests):
        print("stress testing %s" % (stress_tests[i]))
        os.system(stress_tests[i])
        i += 1
    i = 0
    n += 1