/**
 * @file The redirectors model.
 * @author Umar Abdul
 */

import mongoose from 'mongoose';

const rdSchema = mongoose.Schema({
  url: {
    type: String,
    required: true
  }
});

const Redirector = mongoose.model('redirectors', rdSchema);
export default Redirector;

/**
 * Add a new redirector to the records.
 * @param {string} url - The redirector's full URL.
 * @return {object} The newly created redirector.
 */
export const addRedirector = async (url) => {

  url = url.toString();
  if (!(url.startsWith("http://") || url.startsWith("https://")))
    throw new Error("Unknown protocol. Only HTTP(s) are supported!");
  while (url.endsWith("/"))
    url = url.substr(0, url.length - 1).trim()
  if (!url)
    throw new Error("Invalid URL!");
  if (await Redirector.findOne({url}))
    throw new Error("Redirector already exists!");
  const rd = new Redirector({url});
  await rd.save();
  global.socketServer.emit("new_redirector", rd);
  return rd;
};

/**
 * Delete a redirector from the records.
 * @param {string} id - The ID of the redirector.
 * @return {object} The query result.
 */
export const deleteRedirector = async (id) => {

  id = id.toString();
  const data = await Redirector.deleteOne({_id: id});
  if (data.deletedCount === 0)
    throw new Error("Invalid ID!");
  global.socketServer.emit("redirector_deleted", id);
  return data;
};

/**
 * Return a list of all redirectors.
 * @return {object} A list of all redirectors.
 */
export const getRedirectors = async () => {
  return (await Redirector.find({})).reverse();
};
