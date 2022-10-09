/**
 * @file Controller code for redirectors.
 * @author Umar Abdul
 */

import * as model from '../models/redirector.js';

export const addRedirector = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  model.addRedirector(req.body.url).then(data => {
    return res.json({success: "Redirector added!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const deleteRedirector = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  model.deleteRedirector(req.params.id).then(data => {
    return res.json({success: "Redirector deleted!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const getRedirectors = (req, res) => {

  if (req.session.loggedIn !== true)
    return res.status(403).json({error: "Permission denied!"});
  model.getRedirectors().then(data => {
    return res.json(data);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};
