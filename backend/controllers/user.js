/**
 * @file Controller code for the user API
 * @author Umar Abdul
 */

import User, * as model from '../models/user.js';
import {logStatus, logWarning, logError} from '../models/log.js';

const PERM_ERROR = {error: "Permission denied!"}

export const createUser = (req, res) => {
  
  if (req.body.regKey !== global.REGISTRATION_KEY)
    return res.status(403).json({error: "Invalid registration key!"});
  model.createUser(req.body.username, req.body.password).then(user => {
    return res.json({success: "User created!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const deleteUser = (req, res) => {

  if (req.session.admin !== true)
    return res.status(403).json(PERM_ERROR);
  model.deleteUser(req.params.username).then(() => {
    if (global.socketObjects[req.session.username])
      global.socketObjects[req.session.username].disconnect(true);
    return res.json({success: "User deleted!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
}

export const loginUser = (req, res) => {

  model.loginUser(req.body.username, req.body.password).then(async (user) => {
    model.setupSession(req.session, user);
    const token = await model.createToken(user.username);
    global.socketServer.emit("new_teamchat_message", {message: `***** User '${req.session.username}' has logged in! *****`});
    return res.json({
      username: user.username,
      admin: user.admin,
      token
    });
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const logoutUser = async (req, res) => {
  
  if (req.session.loggedIn){
    const username = req.session.username;
    const sock = global.socketObjects[username];
    if (sock)
      sock.disconnect(true); // Close the web socket connection. Will trigger the 'disconnect' handler in ws-server.js
    model.deleteToken(username);
    const user = await model.getUser(req.session.username);
    req.session.destroy();
    user.loggedIn = false;
    logStatus(`User logged out: ${user.username}`);
    global.adminWSEmit("user_updated", user);
    return res.json({success: "You have been logged out!"});
  }else{
    return res.status(403).json(PERM_ERROR);
  }
};

export const getUsers = (req, res) => {

  if (req.session.admin !== true)
    return res.status(403).json(PERM_ERROR);
  model.getUsers().then(users => {
    return res.json(users);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const grantAdmin = (req, res) => {

  if (req.session.admin !== true)
    return res.status(403).json(PERM_ERROR);
  model.grantAdmin(req.params.username, req.session.username).then(() => {
    return res.json({success: "Admin privileges granted!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const revokeAdmin = (req, res) => {

  if (req.session.admin !== true)
    return res.status(403).json(PERM_ERROR);
  model.revokeAdmin(req.params.username, req.session.username).then(() => {
    return res.json({success: "Admin privileges revoked!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const suspendUser = (req, res) => {

  if (req.session.admin !== true)
    return res.status(403).json(PERM_ERROR);
  model.suspendUser(req.params.username, req.session.username).then(() => {
    return res.json({success: "Account suspended!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const activateUser = (req, res) => {
  
  if (req.session.admin !== true)
    return res.status(403).json(PERM_ERROR);
  model.activateUser(req.params.username, req.session.username).then(() => {
    return res.json({success: "Account activated!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};

export const resetPassword = (req, res) => {

  if (req.session.admin !== true)
    return res.status(403).json(PERM_ERROR);
  model.resetPassword(req.params.username, req.body.password, req.session.username).then(() => {
    return res.json({success: "Password changed!"});
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};
