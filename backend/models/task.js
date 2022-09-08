/**
 * @file Tasks model
 * @author Umar Abdul
 */

import mongoose from 'mongoose';
import {v4 as uuid} from 'uuid';

const taskSchema = mongoose.Schema({
  uid: {
    type: String,
    required: true,
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
 * Get all queued task for the agent with the given ID.
 * It DOES NOT update the state of the task.
 * @param {string} agentID - A unique identifier for the agent.
 * @return {object} Available tasks. 
 */
export const getTasks = async (agentID) => {
  return await Task.find({agentID: agentID.toString()});
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
