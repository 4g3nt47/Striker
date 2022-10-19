/**
 * @file Controllers for auth keys.
 * @author Umar Abdul
 */

import * as model from '../models/key.js';
import {logStatus, logWarning, logError} from '../models/log.js';

export const createKey = (req, res) => {
  
  if (!req.session.loggedIn)
    return res.status(403).json({error: "Permission denied!"});
  model.createKey(req.body.key, req.body.keyType, req.session.username).then(data => {
    logStatus(`New auth key created by '${req.session.username}': '${req.body.key}'`);
    return res.json({success: "Key created!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const deleteKey = (req, res) => {

  if (!req.session.loggedIn)
    return res.status(403).json({error: "Permission denied!"});
  model.deleteKey(req.params.key).then(data => {
    logWarning(`Auth key deleted by '${req.session.username}': '${req.params.key}`);
    return res.json({success: "Key deleted!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const getKeys = (req, res) => {

  if (!req.session.loggedIn)
    return res.status(403).json({error: "Permission denied!"});
  model.getKeys().then(keys => {
    return res.json(keys.reverse());
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};
