/**
 * @file Model file for event logs.
 * @author Umar Abdul
 */

import mongoose from 'mongoose';

const logSchema = mongoose.Schema({
  logType: {
    type: Number,
    required: true
  },
  date: {
    type: Number,
    required: true
  },
  message: {
    type: String,
    required: true
  }
});

const Log = mongoose.model("logs", logSchema);
export default Log;

/**
 * Create a status log.
 * @param {string} msg - The log message.
 */
export const logStatus = async (msg) => {
  
  const log = new Log();
  log.logType = 0;
  log.date = Date.now();
  log.message = msg.toString();
  await log.save();
  global.adminWSEmit("new_log", log);
  return log;
};

/**
 * Create a warning log.
 * @param {string} msg - The log message.
 */
export const logWarning = async (msg) => {
  
  const log = new Log();
  log.logType = 1;
  log.date = Date.now();
  log.message = msg.toString();
  await log.save();
  global.adminWSEmit("new_log", log);
  return log;
};

/**
 * Create an error log.
 * @param {string} msg - The log message.
 */
export const logError = async (msg) => {
  
  const log = new Log();
  log.logType = 2;
  log.date = Date.now();
  log.message = msg.toString();
  await log.save();
  global.adminWSEmit("new_log", log);
  return log;
};

/**
 * Get available logs.
 * @param {number} count - The number of logs to fetch.
 * @return {object} An array of logs, with newer ones placed first.
 */
export const getLogs = async (count) => {
  return await Log.find({}, null, {limit: parseInt(count), sort: {date: -1}});
};
