/**
 * @file Routes for auth keys.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {createKey, deleteKey, getKeys} from '../controllers/key.js';

const router = Router();

router.post("/", createKey);
router.delete("/:key", deleteKey);
router.get("/", getKeys);

export default router;
