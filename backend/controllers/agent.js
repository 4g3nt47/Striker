/**
 * @file Controller code for the agent-related ops.
 * @author Umar Abdul
 */

import crypto from 'crypto';
import path from 'path';
import multer from 'multer';
import Agent, * as agentModel from '../models/agent.js';
import Task, * as taskModel from '../models/task.js';
import File, * as fileModel from '../models/file.js';

// A generic error message for requests that got denied due to perm issues.
const PERM_ERROR = {error: "Permission denied!"};

// Configure multer for file upload.
const orgFilenames = {}; // For temporarily mapping unique file IDs to their original name.

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
  agentModel.getAgent(req.params.agentID).then(agent => {
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
    const agentID = req.params.agentID;
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
  const agentID = req.params.agentID;
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

  const agentID = req.params.agentID;
  const results = req.body;
  if (!(results && results.length !== 0))
    return res.json({});
  for (let result of results){
    try{
      await taskModel.setResult(agentID, result);
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
  const agentID = req.params.agentID;
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
  const agentID = req.params.agentID;
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
  const agentID = req.params.agentID;
  try{
    await agentModel.deleteAgent(agentID, req.session.username);
    await taskModel.deleteAllTasks(agentID);
    await fileModel.deleteAllFiles(agentID);
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

/**
 * For obtaining all files for an agent. For logged in users only.
 */
export const getAgentFiles = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json(PERM_ERROR);
  const agentID = req.params.agentID;
  fileModel.getAgentFiles(agentID).then(files => {
    return res.json(files);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Used by both agents and users to upload files to the server.
 */
export const uploadFile = (req, res) => {
  
  const socketServer = global.socketServer;
  const agentID = req.params.agentID;
  const userUpload = req.session.loggedIn;
  const storage = multer.diskStorage({
    destination: global.UPLOAD_LOCATION,
    filename: (req, file, callback) => {
      let orgName = path.basename(path.resolve(file.originalname)).trim();
      if (orgName.length === 0)
        orgName = "untitled";
      const uid = crypto.randomBytes(16).toString('hex');
      orgFilenames[uid] = orgName;
      callback(null, uid);
    }
  });
  const uploader = multer({
    storage,
    limits: {
      fileSize: global.MAX_UPLOAD_SIZE
    }
  }).single('file');
  agentModel.getAgent(agentID).then(agent => {
    uploader(req, res, async (error) => {
      if (error){
        console.log("Upload error:");
        console.log(err);
        return res.status(403).json({error: error.message});
      }
      const uid = path.basename(req.file.path);
      const orgName = orgFilenames[uid];
      const file = new File({
        uid,
        agentID: agentID,
        name: orgName,
        dateCreated: Date.now()
      });
      await file.save();
      delete orgFilenames[uid];
      let msg = userUpload ? (global.serverPrompt + `File received from user '${req.session.username}': ${uid} - ${orgName}`) : (global.serverPrompt + `File received from agent: ${uid} - ${orgName}`);
      socketServer.emit("agent_console_output", {
        agentID: agentID.toString(),
        msg
      });
      // Task agent to download the file if user is the one uploading.
      if (userUpload){
        const task = {
          agentID: agentID.toString(),
          taskType: "upload",
          data: {
            "fileID": uid,
            "name": orgName
          }
        }
        taskModel.createTask(req.session.username, task);
      }      
      return res.json({success: "File uploaded!"});
    });
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Download a file using it's ID.
 */
export const downloadFile = async (req, res) => {

  const fileID = req.params.fileID;
  const file = await File.findOne({uid: fileID});
  if (!file)
    return res.status(404).json({error: "Invalid file!"});
  res.download(`${global.UPLOAD_LOCATION}${fileID}`, file.name, (err) => {
    if (err)
      console.log(err);
  });
};
