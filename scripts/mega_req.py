#!/usr/bin/env python3

import time
import os

print("NORMAL REQUEST SCRIPT")

request1 = "bin/sdstore proc-file ./teste.txt ./shrek_out bcompress gcompress encrypt"

print("[1/2] %s" % request1)

start_time = time.time()
os.system(request1)
end_time = time.time()

print("[2/2] execution time: %s" % (end_time - start_time))