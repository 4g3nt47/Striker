/**
 * @file The team chat messages model.
 * @author Umar Abdul
 */

import mongoose from 'mongoose';

const chatSchema = mongoose.Schema({
  username: {
    type: String,
    required: true
  },
  message: {
    type: String,
    required: true,
  },
  date: {
    type: Number,
    required: true
  }
});

const Chat = mongoose.model("chats", chatSchema);
export default Chat;

/**
 * Create a new message.
 * @param {string} username - The sender's username.
 * @param {string} message - The message string.
 * @return {object} The query result.
 */
export const createMessage = async (username, message) => {

  username = username.toString();
  message = message.toString().trim();
  if (message.length === 0)
    throw new Error("Invalid message!");
  if (message.length > 1024)
    message = message.substr(0, 1024);
  const data = {
    username,
    message,
    date: Date.now()
  };
  const chat = new Chat(data);
  const res = await chat.save();
  global.socketServer.emit("new_teamchat_message", data);
  return res;
};

/**
 * Get all messages.
 * @return {object} An array of messages.
 */
export const getMessages = async () => {
  return await Chat.find({});
};
