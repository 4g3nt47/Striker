#!/usr/bin/python3

import sys, socket, threading

class Redirector:

  def __init__(self, lhost, lport, rhost, rport):
    self.lhost = lhost
    self.lport = int(lport)
    self.rhost = rhost
    self.rport = int(rport)
    self.abort = False

  def start(self):
    print("[*] Starting redirector on %s:%d..." %(self.lhost, self.lport))
    server = socket.socket()
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
      server.bind((self.lhost, self.lport))
    except Exception as e:
      print("[-] Error binding socket: %s" %(str(e)))
      return
    server.listen(10)
    print("[+] Redirecting %s:%d => %s:%d..." %(self.lhost, self.lport, self.rhost, self.rport))
    try:
      while True:
        client, addr = server.accept()
        t = threading.Thread(target=self.route, args=(client, addr))
        t.start()
    except KeyboardInterrupt:
      self.abort = True
      server.close()
      return

  def route(self, client, addr):
    server = socket.socket()
    try:
      server.connect((self.rhost, self.rport))
    except Exception as e:
      print("[-] Error connecting to remote host: %s:%d - %s" %(self.rhost, self.rport, str(e)))
      client.close()
      return
    print("[+] Routing %s:%d => %s:%d" %(addr[0], addr[1], self.rhost, self.rport))
    server.settimeout(0.05)
    client.settimeout(0.05)
    blockSize = 1024
    while not self.abort:
      try:
        data = client.recv(blockSize)
        if (len(data) == 0):
          break
        server.send(data)
      except socket.error as e:
        if not str(e).startswith("timed out"):
          break
      try:
        data = server.recv(blockSize)
        if (len(data) == 0):
          break
        client.send(data)
      except socket.error as e:
        if not str(e).startswith("timed out"):
          break
    server.close()
    client.close()
    print("[+] Disconnected %s:%d => %s:%d" %(addr[0], addr[1], self.rhost, self.rport))
    return

if __name__ == '__main__':
  if len(sys.argv) < 3:
    print("[-] Usage: %s <lhost:lport> <rhost:rport>" %(sys.argv[0]))
    exit(1)
  lhost = sys.argv[1].split(":")[0]
  lport = int(sys.argv[1].split(":")[1])
  rhost = sys.argv[2].split(":")[0]
  rport = int(sys.argv[2].split(":")[1])
  redirector = Redirector(lhost, lport, rhost, rport);
  redirector.start()
