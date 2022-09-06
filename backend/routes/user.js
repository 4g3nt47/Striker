/**
 * @file API routes for user-related ops.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {createUser, loginUser} from '../controllers/user.js';

const router = Router();

router.post("/register", createUser);
router.post("/login", loginUser);

export default router;
