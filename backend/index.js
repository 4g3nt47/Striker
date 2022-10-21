/**
 * @file The server's entrypoint.
 * @author Umar Abdul
 */

import dotenv from 'dotenv';
dotenv.config();

import fs from 'fs';
import https from 'https';
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
import chatRoute from './routes/chat.js';
import keyRouter from './routes/key.js';
import rdRouter from './routes/redirector.js';
import logRouter from './routes/log.js';

// Setup global configs.
const DB_URL = process.env.DB_URL;
const HOST = process.env.HOST || "127.0.0.1";
const PORT = process.env.PORT || 3000;
const SECRET = process.env.SECRET;
global.ORIGIN_URL = process.env.ORIGIN_URL;
global.REGISTRATION_KEY = process.env.REGISTRATION_KEY;
global.MAX_UPLOAD_SIZE = parseInt(process.env.MAX_UPLOAD_SIZE);
global.UPLOAD_LOCATION = process.env.UPLOAD_LOCATION || "static/";
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
      const user = await User.findOne({username: req.session.username, suspended: false});
      if (!user)
        throw new Error("Invalid user: " + req.session.username);
      setupSession(req.session, user);
    }catch(error){
      req.session.destroy();
      return res.status(500).json({error: "Your session has expired!"});
    }
  }
  next();
});

// Mount routes
app.use("/user", userRoute);
app.use("/agent", agentRoute);
app.use("/chat", chatRoute);
app.use("/key", keyRouter);
app.use("/redirector", rdRouter);
app.use("/log", logRouter);

// 404
app.all("*", (req, res) => {
  return res.status(404).json({error: "Invalid route!"});
});

// Error handler. TODO: This is not getting triggered. Fix it.
app.use((error, req, res, next) => {
  console.log(error);
  return res.status(500).json({error: error.message});
});

// Load SSL keys, if defined.
let sslKey = null, sslCert = null;
if (process.env.SSL_KEY && process.env.SSL_CERT){
  output("Loading SSL key and cert...");
  try{
    sslKey = fs.readFileSync(process.env.SSL_KEY);
    sslCert = fs.readFileSync(process.env.SSL_CERT);      
  }catch(error){
    output(`Error: ${error.message}`);
    process.exit(1);
  }
}

// Start the API server
output("Connecting to backend database...");
mongoose.connect(DB_URL).then(() => {
  
  output(`Starting web server on ${HOST}:${PORT}`);
  if (process.env.SSL_KEY && process.env.SSL_CERT){
    const httpsServer = https.createServer({
      key: sslKey,
      cert: sslCert
    }, app);
    httpsServer.listen(PORT, HOST, () => {
      output("Setting up WS server...");
      setupWS(httpsServer);
      output("Server started successfully!");
    });
  }else{
    const httpServer = app.listen(PORT, HOST, () => {
      output("Setting up WS server...");
      setupWS(httpServer);
      output("Server started successfully!");
    });    
  }
});
