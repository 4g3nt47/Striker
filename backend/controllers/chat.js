/**
 * @file Controllers for team server chats.
 * @author Umar Abdul
 */

import * as model from '../models/chat.js';

export const getMessages = (req, res) => {

  if (!req.session.loggedIn)
    return res.status(403).json({error: "Permission denied!"});
  model.getMessages(250).then(messages => {
    return res.json(messages);
  }).catch(error => {
    return res.status(403).json({error: error.message});
  });
};