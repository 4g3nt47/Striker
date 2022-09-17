/**
 * @file Agents model
 * @author Umar Abdul
 */

import mongoose from 'mongoose';
import crypto from 'crypto';

const agentSchema = mongoose.Schema({
  uid: {
    type: String,
    required: true
  },
  delay: {
    type: Number,
    required: true,
    default: 5
  },
  os: {
    type: String,
    required: true
  },
  host: {
    type: String,
    required: true
  },
  user: {
    type: String,
    required: true
  },
  cwd: {
    type: String,
    required: true
  },
  pid: {
    type: Number,
    required: true
  },
  data: {
    type: Object,
    required: false
  },
  dateCreated: {
    type: Number,
    required: true
  },
  lastSeen: {
    type: Number,
    required: true
  },
  frozen: {
    type: Boolean,
    required: true,
    default: false
  }
});

const Agent = mongoose.model('agent', agentSchema);
export default Agent;

/**
 * Create a new agent. Emits the "new_agent" ws event on success.
 * @param {object} data - Agent data as received from the agent.
 * @return {object} The configuration data to send to the agent `config`, and the agent db object. 
 */
export const createAgent = async (data) => {

  const socketServer = global.socketServer;
  const uid = crypto.randomBytes(8).toString('hex');
  const agent = new Agent({
    uid,
    delay: global.AGENT_DELAY,
    os: (data.os ? data.os.toString() : "unknown"),
    host: (data.host ? data.host.toString() : "unknown"),
    user: (data.user ? data.user.toString() : "unknown"),
    cwd: (data.cwd ? data.cwd.toString() : "unknown"),
    pid: (data.pid !== undefined ? parseInt(data.pid) : -1),
    dateCreated: Date.now(),
    lastSeen: Date.now()
  });
  await agent.save();
  socketServer.emit("new_agent", agent);
  const config = {
    uid: agent.uid,
    delay: agent.delay,
  };
  return {config, agent};
};

/**
 * Get all available agents, with newer agents placed first.
 */
export const getAgents = async () => {
  return (await Agent.find({})).reverse();
};

/**
 * Get an agent with the given ID.
 * @param {string} agentID - ID of the agent.
 * @return {object} The target agent
 */
export const getAgent = async (agentID) => {
  
  const agent = await Agent.findOne({uid: agentID.toString()});
  if (!agent)
    throw new Error("Invalid agent!");
  return agent;
};

/**
 * Update the last seen time of an agent. Emits the "update_agent" ws event on success.
 * Should be called whenever an agent checks in to receive new tasks.
 * @param {string} agentID - ID of the agent.
 * @return {object} The updated agent.
 */
export const updateLastSeen = async (agentID) => {
  
  const socketServer = global.socketServer;
  const agent = await Agent.findOne({uid: agentID.toString()});
  if (!agent)
    throw new Error("Invalid agent!");
  agent.lastSeen = Date.now();
  await agent.save();
  socketServer.emit("update_agent", agent);
  return agent;
};

/**
 * Freeze an agent. Emits the "update_agent" ws event on success and also update users console.
 * @param {string} agentID - The ID of the agent.
 * @param {string} username - The user requesting the action.
 * @return {object} The frozen agent.
 */
export const freezeAgent = async (agentID, username) => {
  
  const socketServer = global.socketServer;
  const agent = await Agent.findOne({uid: agentID.toString()});
  if (!(agent && agent.frozen === false))
    throw new Error("Invalid or already frozen agent!");
  agent.frozen = true;
  await agent.save();
  socketServer.emit("update_agent", agent);
  socketServer.emit("agent_console_output", {
    agentID: agentID.toString(),
    msg: global.serverPrompt + `Agent frozen by ${username}`
  });
  return agent;
};

/**
 * Unfreeze an agent. Emits the "update_agent" ws event on success and also update users console.
 * @param {string} agentID - The ID of the agent.
 * @param {string} username - The user requesting the action.
 * @return {object} The unfrozen agent.
 */
export const unfreezeAgent = async (agentID, username) => {

  const socketServer = global.socketServer;
  const agent = await Agent.findOne({uid: agentID.toString()});
  if (!(agent && agent.frozen === true))
    throw new Error("Invalid or already unfrozen agent!");
  agent.frozen = false;
  await agent.save();
  socketServer.emit("update_agent", agent);
  socketServer.emit("agent_console_output", {
    agentID: agentID.toString(),
    msg: global.serverPrompt + `Agent unfrozen by '${username}'`
  });
  return agent;
};

/**
 * Delete an agent. Emits the "agent_deleted" ws event on success.
 * Warning: This DOES NOT delete existing tasks. Caller should also call the `deleteAllTasks` of the task model.
 * @param {string} agentID - The ID of the target agent.
 * @param {string} username - The user requesting the action.
 * @return {object} The query result.
 */
export const deleteAgent = async (agentID, username) => {

  const socketServer = global.socketServer;
  const data = await Agent.deleteOne({uid: agentID.toString()});
  if (data.deletedCount === 0)
    throw new Error("Invalid agent!");
  socketServer.emit("agent_deleted", {
    agentID: agentID.toString(),
    user: username
  });
  return data;
};
