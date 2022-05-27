
#!/usr/bin/env python3

import time
import os

print("SINGLE REQUEST SCRIPT")

request1 = "bin/sdstore proc-file ./teste.txt ./teste_out bcompress"

print("[1/2] %s" % request1)

start_time = time.time()
os.system(request1)
end_time = time.time()

print("execution time: %s" % (end_time - start_time))