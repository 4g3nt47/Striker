/**
 * @file Controllers for event logs.
 * @author Umar Abdul
 */

import * as model from '../models/log.js';

export const getLogs = (req, res) => {

  if (req.session.admin !== true)
    return res.status(403).json({error: "Permission denied!"});
  model.getLogs(250).then(logs => {
    return res.json(logs);
  }).catch(error => {
    return res.status(403).json(logs);
  });
};
