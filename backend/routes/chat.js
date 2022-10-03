/**
 * @file Chat routes.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {getMessages} from '../controllers/chat.js';

const router = Router();

router.get("/messages", getMessages);

export default router;
