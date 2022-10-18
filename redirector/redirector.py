#!/usr/bin/python3
#---------------------------------------------------------------------------------------
#      A dumb pipe redirector for Striker C2. This on it's own is NOT good/recommended
# way to route traffic to the C2 server. See the README.md
#                                                                     Author: Umar Abdul
#---------------------------------------------------------------------------------------

import sys, socket, threading

class Redirector:

  # Constructor.
  # `lhost` - Host to listen on.
  # `lport` - Port to listen on.
  # `rhost` - Host to forward connections to.
  # `rport` - Port to forward connections to.
  def __init__(self, lhost, lport, rhost, rport):
    self.lhost = lhost
    self.lport = int(lport)
    self.rhost = rhost
    self.rport = int(rport)
    self.abort = False

  # Start the redirector.
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
    print("[+] Listening for connections...")
    try:
      while True:
        client, addr = server.accept()
        t = threading.Thread(target=self.route, args=(client, addr))
        t.start()
    except KeyboardInterrupt:
      self.abort = True
      server.close()
      return

  # Route a connected client.
  # `client` - The socket object of the client.
  # `addr` - The IP address and port of the client.
  def route(self, client, addr):
    server = socket.socket()
    try:
      server.connect((self.rhost, self.rport))
    except Exception as e:
      print("[-] Error connecting to remote host: %s:%d - %s" %(self.rhost, self.rport, str(e)))
      client.close()
      return
    print("[+] Routing %s:%d => %s:%d..." %(addr[0], addr[1], self.rhost, self.rport))
    # Small timeouts for faster routing.
    server.settimeout(0.005)
    client.settimeout(0.005)
    # Max bytes to read at a time from server/client. Need to be large or file uploads/downloads will be slow.
    blockSize = 999999
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
