/**
 * @file Auth key model.
 * @author Umar Abdul.
 */

import mongoose from 'mongoose';

const keySchema = mongoose.Schema({
  key: {
    type: String,
    required: true
  },
  keyType: {
    type: Number,
    required: true,
  },
  owner: {
    type: String,
    required: true
  },
  useCount: {
    type: Number,
    required: true,
    default: 0
  },
  creationDate: {
    type: Number,
    required: true,
  }
});

const Key = mongoose.model("keys", keySchema);
export default Key;

/**
 * Add a new key.
 * @param {string} key - The key.
 * @param {number} keyType - The type of the key.
 * @param {string} owner - The username of the creator.
 * @return {object} Query result.
 */
export const createKey = async (key, keyType, owner) => {

  key = key.toString();
  keyType = parseInt(keyType);
  owner = owner.toString();
  if (key.length != 32)
    throw new Error("Key must be 32 characters long!");
  if (keyType < 0 || keyType > 1)
    throw new Error("Invalid key type, must be 0 or 1.");
  if (await Key.findOne({key}))
    throw new Error("Key already exist!");
  let data = {
    key,
    keyType,
    owner,
    useCount: 0,
    creationDate: Date.now()
  };
  const newKey = new Key(data);
  const res = await newKey.save();
  global.socketServer.emit("new_authkey", data);
  return res;
};

/**
 * Delete a key.
 * @param {string} key - The target key.
 * @return {object} The query result.
 */
export const deleteKey = async (key) => {
  
  let res = await Key.deleteOne({key: key.toString()});
  if (res.deletedCount > 0)
    global.socketServer.emit("authkey_deleted", key.toString());
  return res;
};

/**
 * Get all keys.
 * @return {object} An array of all available keys.
 */
export const getKeys = async () => {
  return await Key.find({});
};

/**
 * Authenticate the given key. If it's a one time key, it will be deleted.
 * @param key - The key to authenticate.
 * @return {boolean} true on success.
 */
export const authenticate = async (key) => {

  key = key.toString();
  const validKey = await Key.findOne({key});
  if (!validKey)
    return false;
  if (validKey.keyType !== 0){
    deleteKey(key);
  }else{
    validKey.useCount++;
    await validKey.save();
    global.socketServer.emit("authkey_updated", {
      key: validKey.key,
      keyType: validKey.keyType,
      owner: validKey.owner,
      useCount: validKey.useCount,
      creationDate: validKey.creationDate
    });
  }
  return true;
};
