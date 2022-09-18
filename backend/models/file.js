/**
 * @file Files model
 * @author Umar Abdul
 */

import mongoose from 'mongoose';
import crypto from 'crypto';

const fileSchema = mongoose.Schema({
  uid: {
    type: String,
    required: true,
  },
  agentID: {
    type: String,
    required: true
  },
  name: {
    type: String,
    required: true
  },
  dateCreated: {
    type: Number,
    required: true
  }
});

const File = mongoose.model("file", fileSchema);
export default File;

/**
 * Get all files for an agent.
 * @param {string} agentID - The ID of the target agent.
 * @return {object} An array of agent files.
 */
export const getAgentFiles = async (agentID) => {
  return await File.find({agentID: agentID.toString()});
};
