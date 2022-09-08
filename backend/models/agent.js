/**
 * @file Agents model
 * @author Umar Abdul
 */

import mongoose from 'mongoose';
import {v4 as uuid} from 'uuid';

const agentSchema = mongoose.Schema({
  uid: {
    type: String,
    required: true
  },
  dateCreated: {
    type: Number,
    required: true
  },
  lastSeen: {
    type: Number,
    required: true
  },
  info: {
    type: Object,
    required: true
  }
});

const Agent = mongoose.model('agent', agentSchema);
export default Agent;

/**
 * Create a new agent.
 * @param {object} data - Agent info, as received from the agent.
 * @return {object} The configuration data to send to the agent `config`, and the agent db object. 
 */
export const createAgent = async (info) => {

  const uid = uuid();
  const agent = new Agent({
    uid,
    dateCreated: Date.now(),
    lastSeen: Date.now(),
    info
  });
  await agent.save();
  const config = {
    uid,
    delay: 2000
  };
  return {config, agent};
};

/**
 * Get all available agents
 */
export const getAgents = async () => {
  return await Agent.find({});
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
