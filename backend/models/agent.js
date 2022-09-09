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
    required: true
  },
  dateCreated: {
    type: Number,
    required: true
  },
  lastSeen: {
    type: Number,
    required: true
  }
});

const Agent = mongoose.model('agent', agentSchema);
export default Agent;

/**
 * Create a new agent.
 * @param {object} data - Agent data as received from the agent.
 * @return {object} The configuration data to send to the agent `config`, and the agent db object. 
 */
export const createAgent = async (data) => {

  const uid = crypto.randomBytes(8).toString('hex');
  const agent = new Agent({
    uid,
    os: (data.os ? data.os.toString() : "unknown"),
    host: (data.host ? data.host.toString() : "unknown"),
    user: (data.user ? data.user.toString() : "unknown"),
    cwd: (data.cwd ? data.cwd.toString() : "unknown"),
    pid: (data.pid !== undefined ? parseInt(data.pid) : -1),
    dateCreated: Date.now(),
    lastSeen: Date.now(),
    data: {}
  });
  await agent.save();
  const config = {
    uid,
    delay: 2000
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
 * Update the last seen time of an agent.
 * Should be called whenever an agent checks in to receive new tasks.
 * @param {string} agentID - ID of the agent.
 */
export const updateLastSeen = async (agentID) => {
  await Agent.updateOne({uid: agentID.toString()}, {lastSeen: Date.now()});
};
