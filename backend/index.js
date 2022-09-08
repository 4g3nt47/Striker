/**
 * @file The server's entrypoint.
 * @author Umar Abdul
 */

import dotenv from 'dotenv';
dotenv.config();
import express from 'express';
import cors from 'cors';
import mongoose from 'mongoose';
import session from 'express-session';
import MongoStore from 'connect-mongo';
import {Server} from 'socket.io';
import {output} from './lib/utils.js';
import User, {setupSession, getUserByToken} from './models/user.js';
import userRoute from './routes/user.js';
import agentRoute from './routes/agent.js';

// .env
const DB_URL = process.env.DB_URL;
const PORT = process.env.PORT || 3000;
const SECRET = process.env.SECRET;
const ORIGIN_URL = process.env.ORIGIN_URL;

// Setup express
const app = express();
app.use(express.json());
app.use(express.urlencoded({extended: true}));

// cors
app.use(cors({
  origin: ORIGIN_URL,
  credentials: true
}));

// Define our static dir
app.use("/static", express.static("./static"));

// Setup session manager
app.set('trust proxy', 1); // In case of local proxies that use HTTP
app.use(session({
  secret: SECRET,
  resave: true,
  saveUninitialized: false,
  store: MongoStore.create({
    mongoUrl: DB_URL,
    ttl: 60 * 60 * 24 * 7,
    crypto: {
      secret: SECRET
    }
  })
}));

// A middleware for auto-loading user profile for every request. Helps keep session data updated.
app.use(async (req, res, next) => {

  if (req.session.loggedIn){
    try{
      const user = await User.findOne({username: req.session.username});
      setupSession(req.session, user);
    }catch(error){
      req.session.destroy();
      return res.status(500).json({error: "Error loading session: " + error.message});
    }
  }
  next();
});

// Mount routes

app.use("/user", userRoute);
app.use("/agent", agentRoute);

// 404
app.all("*", (req, res) => {
  return res.status(404).json({error: "Invalid route!"});
});

// Error handler
app.use((error, req, res, next) => {
  console.log(error);
  return res.status(500).json({error: error.message});
});

// Start the API server
output("Connecting to backend database...");
mongoose.connect(DB_URL).then(() => {
  
  output("Starting API server...");
  const httpServer = app.listen(PORT, () => {
    output("Server started on port: " + PORT);
  });

  /**
   * Setup the ws server
   */

  // Create the ws server, and make it global.
  global.socketServer = new Server(httpServer, {
    cors: {
      origin: ORIGIN_URL,
      methods: ["GET", "POST"]
    }
  });

  // Define an auth middleware.
  socketServer.use((socket, next) => {
    const token = socket.handshake.auth.token;
    if (!token)
      return next(new Error("Access denied!"));
    next();
  });

  // Handle connections.

  socketServer.on('connection', (client) => {
    output("New ws connection: " + client.id);
  });
});
