/**
 * @file Tasks model
 * @author Umar Abdul
 */

import crypto from 'crypto';
import mongoose from 'mongoose';
import Agent, {getAgent} from './agent.js';

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
 * Create a new task.
 * @param {string} owner - The task owner (username of the user that creates the task)
 * @param {object} data - The request body received.
 * @return {object} The task created.
 */
export const createTask = async (owner, data) => {

  const taskID = crypto.randomBytes(8).toString('hex').trim();
  let agentID = data.agentID.toString().trim();
  const agent = await getAgent(agentID); // will throw an error in not valid.
  const taskType = data.taskType.toString().trim();
  const task = new Task({
    uid: taskID,
    owner: owner,
    agentID: agent.uid,
    taskType,
    data: data.data,
    dateCreated: Date.now()
  });
  await task.save();
  return task;
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
  for (let task of data)
    allTasks[task.agentID].unshift(task);
  return allTasks;
};

/**
 * Mark a task as received.
 * @param {string} taskID - The ID of the task.
 * @return {object} The query result.
 */
export const markReceived = async (taskID) => {
  
  const data = await Task.updateOne({uid: taskID.toString()}, {received: true, dateReceived: Date.now()});
  if (data.modifiedCount < 1)
    throw new Error("Invalid task!");
  return data;
};

/**
 * Set the result of a task and mark it as complete.
 * @param {string} taskID - The ID of the task.
 * @param {string} result - The result of the task.
 * @return {object} The query result.
 */
export const setResult = async (taskID, result) => {

  const data = await Task.updateOne({uid: taskID.toString()}, {completed: true, dateCompleted: Date.now(), result: result.toString()});
  if (data.modifiedCount < 1)
    throw new Error("Invalid task!");
  return data;
};
