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
import {setupWS} from './lib/ws-server.js';
import {output} from './lib/utils.js';
import User, {setupSession} from './models/user.js';
import userRoute from './routes/user.js';
import agentRoute from './routes/agent.js';

// Setup global configs.
const DB_URL = process.env.DB_URL;
const PORT = process.env.PORT || 3000;
const SECRET = process.env.SECRET;
global.ORIGIN_URL = process.env.ORIGIN_URL;
global.REGISTRATION_KEY = process.env.REGISTRATION_KEY;
global.AGENT_DELAY = parseInt(process.env.AGENT_DELAY);
global.MAX_UPLOAD_SIZE = parseInt(process.env.MAX_UPLOAD_SIZE);
global.UPLOAD_LOCATION = process.env.UPLOAD_LOCATION;
if (!global.UPLOAD_LOCATION.endsWith("/"))
  global.UPLOAD_LOCATION += "/";

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
  cookie: {
    expires: 1000 * 60 * 60 * 24 * 7
  },
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
      if (!user)
        throw new Error("Invalid user: " + req.session.username);
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

  setupWS(httpServer);

});
