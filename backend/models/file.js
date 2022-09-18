/**
 * @file Files model
 * @author Umar Abdul
 */

import fs from 'fs';
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

/**
 * Delete a file.
 * @param {string} agentID - The ID of the agent.
 * @param {string} fileID - The ID of the file.
 * @return {object} The result of the delete query.
 */
export const deleteFile = async (agentID, fileID) => {

  agentID = agentID.toString();
  fileID = fileID.toString();
  const file = await File.findOne({agentID, uid: fileID});
  if (!file)
    throw new Error("Invalid file!");
  const loc = global.UPLOAD_LOCATION + fileID;
  fs.unlink(loc, () => {});
  return await File.deleteOne({agentID, uid: fileID});
};

/**
 * Delete all files for an agent.
 * @param {string} agentID - The ID of the agent.
 */
export const deleteAllFiles = async (agentID) => {

  const files = await getAgentFiles(agentID);
  for (let file of files)
    await deleteFile(agentID, file.uid);
};
