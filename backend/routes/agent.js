/**
 * @file The API routes for agents-related ops.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {
  getAgent, getAgents, agentInit, freezeAgent, unfreezeAgent, deleteAgent, agentPing,
  createTask, getAllTasks, getPendingTasks, getTasks, setTasksResults, deleteTask,
  uploadFile, reuploadFile, downloadFile, getAgentFiles, deleteFile
} from '../controllers/agent.js';

const router = Router();

// Task routes.
router.post("/tasks", createTask);
router.get("/tasks", getAllTasks);
router.get("/tasks/:agentID", getPendingTasks);
router.get("/tasks/:agentID/all", getTasks);
router.post("/tasks/:agentID", setTasksResults);
router.delete("/tasks/:agentID/:taskID", deleteTask);

// Agent routes
router.get("/", getAgents);
router.get("/:agentID", getAgent);
router.post("/init", agentInit);
router.get("/freeze/:agentID", freezeAgent);
router.get("/unfreeze/:agentID", unfreezeAgent);
router.delete("/:agentID", deleteAgent);
router.get("/ping/:agentID", agentPing);

// File routes
router.get("/files/:agentID", getAgentFiles);
router.post("/upload/:agentID/:taskID", uploadFile);
router.get("/upload/:agentID/:fileID", reuploadFile);
router.get("/download/:fileID", downloadFile);
router.delete("/file/:agentID/:fileID", deleteFile);

export default router;
