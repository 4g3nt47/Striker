/**
 * @file The API routes for agents-related ops.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {
  getAgent, getAgents, agentInit, freezeAgent, unfreezeAgent, deleteAgent,
  createTask, getAllTasks, getPendingTasks, getTasks, setTasksResults, deleteTask,
  uploadFile, getAgentFiles
} from '../controllers/agent.js';

const router = Router();

// Task routes.
router.post("/tasks", createTask);
router.get("/tasks", getAllTasks);
router.get("/tasks/:uid", getPendingTasks);
router.get("/tasks/:uid/all", getTasks);
router.post("/tasks/:uid", setTasksResults);
router.delete("/tasks/:uid", deleteTask);

// Agent routes
router.get("/", getAgents);
router.get("/:uid", getAgent);
router.post("/init", agentInit);
router.get("/freeze/:uid", freezeAgent);
router.get("/unfreeze/:uid", unfreezeAgent);
router.delete("/:uid", deleteAgent);

// File routes
router.get("/files/:uid", getAgentFiles);
router.post("/upload/:agentID/:taskID", uploadFile);

export default router;
