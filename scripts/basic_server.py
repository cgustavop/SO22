#!/usr/bin/env python3

import os

print("---- SERVER SCRIPT ----")
print("[1/5] checking if tmp/ exits")

if not os.path.isdir("tmp"):
    os.makedirs("tmp")
    print("[2/5] creating folder tmp/")
else:
    print("[2/5] folder tmp exists")

print("[3/5] make clean")
os.system("make clean")
print("[4/5] make")
os.system("make")

server = "bin/sdstored config.txt exec/"

print("[5/5] %s" % server)

print("The server is up an running, time to make requests")
os.system(server)