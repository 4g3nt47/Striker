/**
 * @file API routes for user-related ops.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {
  createUser, loginUser, logoutUser, getUsers
} from '../controllers/user.js';

const router = Router();

router.get("/", getUsers);
router.post("/register", createUser);
router.post("/login", loginUser);
router.get("/logout", logoutUser);

export default router;
