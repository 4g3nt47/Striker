/**
 * @file API routes for user-related ops.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {
  createUser, deleteUser, loginUser, logoutUser, getUsers,
  grantAdmin, revokeAdmin, suspendUser, activateUser
} from '../controllers/user.js';

const router = Router();

router.get("/", getUsers);
router.post("/register", createUser);
router.delete("/:username", deleteUser);
router.post("/login", loginUser);
router.get("/logout", logoutUser);

router.get("/admin/grant/:username", grantAdmin);
router.get("/admin/revoke/:username", revokeAdmin);
router.get("/suspend/:username", suspendUser);
router.get("/activate/:username", activateUser);

export default router;
