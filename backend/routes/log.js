/**
 * @file Routes for event logs.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {getLogs} from '../controllers/log.js';

const router = Router();

router.get("/", getLogs);

export default router;
