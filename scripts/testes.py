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

class Testes_indexes(enum.Enum):
   bcompress = 0
   gcompress = 1
   encrypt = 2
   combination1 = 3
   combination2 = 4

   bdecompress = 5
   gdecompress = 6
   decrypt = 7
   reverted1 = 8
   reverted2 = 9

#singular proc tests
bcompress= "bin/sdstore proc-file ./teste.txt ./tests_out/bcompress_out bcompress"
gcompress= "bin/sdstore proc-file ./teste.txt ./tests_out/gcompress_out gcompress"
encrypt= "bin/sdstore proc-file ./teste.txt ./tests_out/encrypt_out encrypt"

#reverse singular proc tests
bdecompress= "bin/sdstore proc-file ./tests_out/bcompress_out ./tests_out/bdecompress_out.txt bdecompress"
gdecompress= "bin/sdstore proc-file ./tests_out/gcompress_out ./tests_out/gdecompress_out.txt gdecompress"
decrypt= "bin/sdstore proc-file ./tests_out/encrypt_out ./tests_out/decrypt_out.txt decrypt"

#combination and reversing tests
combination1= "bin/sdstore proc-file ./teste.txt ./tests_out/combined01_out bcompress gcompress encrypt"
revert1= "bin/sdstore proc-file ./tests_out/combined01_out ./tests_out/reverted1_out.txt decrypt gdecompress bdecompress"

combination2= "bin/sdstore proc-file ./teste.txt ./tests_out/combined02_out encrypt gcompress bcompress encrypt gcompress"
revert2= "bin/sdstore proc-file ./tests_out/combined02_out ./tests_out/reverted2_out.txt gdecompress decrypt bdecompress gdecompress decrypt"

testes = [bcompress, gcompress,encrypt,combination1,combination2,bdecompress,gdecompress,decrypt,revert1,revert2]

total_tests = len(testes)
passed_tests = 0

#run procs
i = 0
while(i<total_tests):
    
    if(i<5):
        print("[%d/%d] testing %s" % (i+1, total_tests, testes[i]))
        start_time1 = time.time()
        os.system(testes[i])
        end_time = time.time()
        execution_time = end_time - start_time1
        print("execution time: %s" % execution_time)
        #os.system(expected[i])
        
    else:
        print("[%d/%d] testing revert %s" % (i+1, total_tests, testes[i]))
        os.system(testes[i])
        if (filecmp.cmp("tests_out/" + Testes_indexes(i).name + "_out.txt", "teste.txt", shallow=True)):
            passed_tests += 1
    
    i += 1





print("[%d/%d] %d/5 test passed" % (i, total_tests, passed_tests))