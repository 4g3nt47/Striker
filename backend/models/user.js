/**
 * @file The user database model
 * @author Umar Abdul
 */

import crypto from 'crypto';
import mongoose from 'mongoose';
import bcrypt from 'bcrypt';
import validator from 'validator';
import {logStatus, logWarning, logError} from './log.js';


// The collection schema.
const userSchema = mongoose.Schema({
  username: {
    type: String,
    required: true
  },
  password: {
    type: String,
    required: true
  },
  admin: {
    type: Boolean,
    required: true,
    default: false
  },
  suspended: {
    type: Boolean,
    required: true,
    default: false,
  },
  token: {
    type: String
  },
  creationDate: {
    type: Number,
    required: true
  },
  lastSeen: {
    type: Number,
    required: true
  }
});

// Methods

/**
 * Set/update user password
 * @param {string} password - The new password
 * @return {string} The updated password hash
 */
userSchema.methods.setPassword = async function(password){
  
  password = password.toString().trim();
  // if (!validator.isStrongPassword(password))
  //   throw new Error("Password too weak!");
  this.password = await bcrypt.hash(password, 10);
  return this.password;
};

/**
 * Check if given password matches that of user. Used for auth.
 * @param {string} password - The password to check
 * @return {boolean} true if it's a match
 */
userSchema.methods.validatePassword = async function(password){
  return await(bcrypt.compare(password, this.password));
};

// Export it
const User = mongoose.model("user", userSchema);
export default User;

/**
 * Create a non-admin user account.
 * @param {string} username - The username
 * @param {string} password - The password
 * @return {object} the created user
 */
export const createUser = async (username, password) => {

  username = username.toString().trim();
  password = password.toString().trim();
  if (username.length < 3 || username.length > 16)
    throw new Error("Invalid username length!");
  if (!validator.isAlphanumeric(username))
    throw new Error("Username must be alphanumeric!");
  if (await User.findOne({username}))
    throw new Error("User already exists!");
  const data = {
    username,
    admin: false,
    suspended: false,
    creationDate: Date.now(),
    lastSeen: Date.now()
  }
  const user = new User(data);
  await user.setPassword(password);
  await user.save();
  logStatus("User account created: " + user.username);
  global.adminWSEmit("new_user", data);
  return user;
};

/**
 * Delete a user account.
 * @param {string} username - The target user's username.
 * @param {string} deleter - The user requesting the action.
 * @return {object} The query result.
 */
export const deleteUser = async (username, deleter) => {

  const data = await User.deleteOne({username: username.toString()});
  if (data.deletedCount == 0)
    throw new Error("Invalid user!");
  logWarning(`User account '${username}' deleted by '${deleter}'`);
  global.adminWSEmit("user_deleted", username);
  return data;
};

/**
 * Login user if not already logged in. Will always fail for suspended users.
 * @param {string} username - The username submitted
 * @param {string} password - The password submitted
 * @return {object} The user's profile
 */
export const loginUser = async (username, password) => {

  username = username.toString().trim();
  password = password.toString().trim();
  const user = await User.findOne({username, suspended: false});
  if (!(user && (await user.validatePassword(password) === true)))
    throw new Error("Authentication failed!");
  if (global.socketObjects[username])
    throw new Error("Already logged in. Please close other sessions to continue.");
  user.lastSeen = Date.now();
  await user.save();
  user.loggedIn = true;
  logStatus("User logged in: " + user.username);
  global.adminWSEmit("user_updated", {
    username,
    admin: false,
    suspended: false,
    creationDate: Date.now(),
    lastSeen: Date.now(),
    loggedIn: true
  });
  return user;
};

/**
 * Create a temporary token for the user that can be used for authentication.
 * This token should be created on login, and invalidated on logout.
 * @param {string} username - Username of the user to create token for.
 * @return {string} The generated token.
 */
export const createToken = async (username) => {
   
  const user = await User.findOne({username: username.toString()});
  if (!user)
    throw new Error("Invalid user!");
  const token = crypto.randomBytes(32).toString('hex');
  user.token = token;
  await user.save();
  return token;
};

/**
 * Delete the token of a user.
 * @param {string} username - The target user
 */
export const deleteToken = async (username) => {

  const user = await User.findOne({username: username.toString()});
  if (!user)
    return;
  user.token = null;
  await user.save();
  return;
};

/**
 * Load data of user with the given token. Won't work for suspended users.
 * @param {string} token - The token to lookup
 * @return {object} The user data.
 */
export const getUserByToken = async (token) => {
  
  const user = await User.findOne({token: token.toString(), suspended: false});
  if (!user)
    throw new Error("Invalid token!");
  return user;
};

/**
 * Setup session data for logged in user.
 * @param {object} session - The session object
 * @param {object} user - The user profile
 */
export const setupSession = async (session, user) => {

  session.username = user.username;
  session.admin = user.admin;
  session.user = user;
  session.loggedIn = true;
};

/**
 * Get the data of a single user.
 * @param {string} username - The user's username.
 * @return {object} - The user data (with some fields removed).
 */
export const getUser = async (username) => {

  const user = await User.findOne({username: username.toString()});
  if (!user)
    return null;
  return {
    username: user.username,
    admin: user.admin,
    suspended: user.suspended,
    creationDate: user.creationDate,
    lastSeen: user.lastSeen,
    loggedIn: global.socketObjects[user.username] !== undefined
  };
};

/**
 * Get data of all users.
 * @return {object} Data of all users.
 */
export const getUsers = async () => {

  const data = await User.find({});
  let users = [];
  for (let user of data){
    users.push({
      username: user.username,
      admin: user.admin,
      suspended: user.suspended,
      creationDate: user.creationDate,
      lastSeen: user.lastSeen,
      loggedIn: global.socketObjects[user.username] !== undefined
    });
  }
  return users;
};

/**
 * Grant admin privs to a user.
 * @param {string} username - The target user's username.
 * @param {string} grantor - The user requesting the action.
 * @return {object} The query result.
 */
export const grantAdmin = async (username, grantor) => {
  
  const data = await User.updateOne({username: username.toString(), admin: false}, {admin: true});
  if (data.modifiedCount > 0){
    logWarning(`Admin privileges granted to user '${username}' by '${grantor}'`);
    global.adminWSEmit("user_updated", await getUser(username.toString()));
  }
  return data;
};

/**
 * Revoke admin privs of a user.
 * @param {string} username - The target user's username.
 * @param {string} revoker - The user requesting the action.
 * @return {object} The query result.
 */
export const revokeAdmin = async (username, revoker) => {
  
  const data = await User.updateOne({username: username.toString(), admin: true}, {admin: false});
  if (data.modifiedCount > 0){
    logWarning(`Admin privileges for user '${username}' revoked by '${revoker}'`);
    global.adminWSEmit("user_updated", await getUser(username.toString()));
  }
  return data;
};

/**
 * Suspend a user account and kill any active session it has.
 * @param {string} username - The target user's username.
 * @param {string} suspender - The user requesting the action.
 * @return {object} The query result.
 */
export const suspendUser = async (username, suspender) => {
  
  const data = await User.updateOne({username: username.toString(), suspended: false}, {suspended: true});
  if (data.modifiedCount > 0){
    logWarning(`User account '${username}' suspended by '${suspender}'`);
    global.adminWSEmit("user_updated", await getUser(username.toString()));
  }
  if (global.socketObjects[username])
    global.socketObjects[username].disconnect(true);
  return data;
};

/**
 * Activate a suspended user account.
 * @param {string} username - The target user's username.
 * @param {string} activator - The user requesting the action.
 * @return {object} The query result.
 */
export const activateUser = async (username, activator) => {
  
  const data = await User.updateOne({username: username.toString(), suspended: true}, {suspended: false});
  if (data.modifiedCount > 0){
    logWarning(`User account '${username}' activated by '${activator}'`);
    global.adminWSEmit("user_updated", await getUser(username.toString()));
  }
  return data;
};

/**
 * Reset the password of a user.
 * @param {string} username - The target user's username.
 * @param {string} password - The new password.
 * @param {string} resetter - The user requesting the action.
 * @return {object} The query result.
 */
export const resetPassword = async (username, password, resetter) => {

  username = username.toString();
  password = password.toString();
  const user = await User.findOne({username});
  if (!user)
    throw new Error("Invalid user!");
  await user.setPassword(password);
  logWarning(`Account password for user '${username}' reset by '${resetter}'`);
  return await user.save();
};
