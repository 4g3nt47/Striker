/**
 * @file Routes for redirectors.
 * @author Umar Abdul
 */

import {Router} from 'express';
import {addRedirector, deleteRedirector, getRedirectors} from '../controllers/redirector.js';

const router = Router();

router.post("/", addRedirector);
router.delete("/:id", deleteRedirector);
router.get("/", getRedirectors);

export default router;
