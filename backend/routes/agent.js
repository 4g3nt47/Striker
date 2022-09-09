/**
 * @file The API routes for agents-related ops.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {
  getAgent, getAgents, agentInit,
  createTask, getAllTasks, getPendingTasks, getTasks, setTasksResults
} from '../controllers/agent.js';

const router = Router();

// Task routes.
router.post("/tasks", createTask);
router.get("/tasks", getAllTasks);
router.get("/tasks/:uid", getPendingTasks);
router.get("/tasks/:uid/all", getTasks);
router.post("/tasks/:uid", setTasksResults);

// Agent routes
router.get("/", getAgents);
router.get("/:uid", getAgent);
router.post("/init", agentInit);


export default router;
