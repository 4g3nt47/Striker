/**
 * @file The API routes for agents-related ops.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {
  getAgent, getAgents, agentInit, getTasks, setTasksResults
} from '../controllers/agent.js';

const router = Router();

router.get("/", getAgents);
router.get("/:uid", getAgent);
router.post("/init", agentInit);

router.get("/tasks/:uid", getTasks);
router.post("/tasks/:uid", setTasksResults);

export default router;
