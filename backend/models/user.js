/**
 * @file The user database model
 * @author Umar Abdul
 */

import crypto from 'crypto';
import mongoose from 'mongoose';
import bcrypt from 'bcrypt';
import validator from 'validator';

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
  const user = new User({
    username,
    creationDate: Date.now(),
    lastSeen: Date.now()
  });
  await user.setPassword(password);
  await user.save();
  return user;
};

/**
 * Login user.
 * @param {string} username - The username submitted
 * @param {string} password - The password submitted
 * @return {object} The user's profile
 */
export const loginUser = async (username, password) => {

  username = username.toString().trim();
  password = password.toString().trim();
  const user = await User.findOne({username});
  if (!(user && (await user.validatePassword(password) === true)))
    throw new Error("Authentication failed!");
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
 * Load data of user with the given token.
 * @param {string} token - The token to lookup
 * @return {object} The user data.
 */
export const getUserByToken = async (token) => {
  
  const user = await User.findOne({token: token.toString()});
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

