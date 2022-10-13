<script>

  /**
   * @file The primary component.
   * @author Umar Abdul
   */

  import {io} from 'socket.io-client';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import Login from './components/Login.svelte';
  import Register from './components/Register.svelte';
  import SuccessMsg from './components/SuccessMsg.svelte';
  import ErrorMsg from './components/ErrorMsg.svelte';
  import AgentsList from './components/agent/AgentsList.svelte';
  import AgentHandler from './components/agent/AgentHandler.svelte';
  import Users from './components/Users.svelte';
  import TeamChat from './components/TeamChat.svelte';
  import AuthKeys from './components/AuthKeys.svelte';
  import Redirectors from './components/Redirectors.svelte';


  // Create a new session tracker.
  const createSession = () => {

    return ({
      api: import.meta.env.VITE_STRIKER_API,
      username: "",
      loggedIn: false,
      admin: false,
      token: "",
      page: "login"
    });
  };

  // Save the session data to local storage.
  const saveSession = () => {

    if (Storage){
      localStorage.setItem("striker", JSON.stringify(session));
      return true;
    }
    return false;
  };

  // Load session from local storage or create a new one.
  const loadSession = () => {

    let newSession = createSession();
    if (!Storage)
      return newSession;
    let savedSession = localStorage.getItem("striker");
    return (savedSession ? JSON.parse(savedSession) : newSession);
  };

  // Handles session update event.
  const updateSession = (e) => {
    session = e.detail;
    saveSession();
  };

  // Switch page.
  const switchPage = (newPage) => {
    session.page = newPage;
    saveSession();
  }

  // Handles page switch event.
  const switchPageHandler = (e) => {
    switchPage(e.detail);
  }

  // Called after a successful login
  const loggedIn = (e) => {

    const user = e.detail;
    session.loggedIn = true;
    session.username = user.username;
    session.admin = user.admin;
    session.token = user.token;
    session.page = "agents";
    setupC2();
    saveSession();
  };

  // Logout the user.
  const logout = async () => {

    fetch(`${session.api}/user/logout`, {
      credentials: "include"
    });
    session = createSession();
    saveSession();
  };

  // Run some setups following authentication.
  const setupC2 = () => {

    wsInit(session.token);
    loadAgents();
    loadRedirectors();
    loadAuthKeys();
    loadTasks();
    loadTeamchatMessages();
    if (session.admin)
      loadUsers();
  };


  // Configure the web socket client and define event handlers.
  const wsInit = (token) => {

    // Connect and authenticate.
    socket = io(session.api, {
      auth: {
        token
      }
    });

    // Handle auth/connection error.
    socket.on('connect_error', (err) => {
      console.log("Error connecting to ws: " + err.message);
    });

    // Called when a new agent is created.
    socket.on('new_agent', (agent) => {

      agents = [agent, ...agents];
      tasks[agent.uid] = [];
      consoleMsgs[agent.uid] = [];
    });

    // Called when an agent is updated.
    socket.on('update_agent', (agent) => {

      for (let i = 0; i < agents.length; i++){
        if (agents[i] === undefined) // For when an agent gets deleted before the function returns (rare)
          continue;
        if (agents[i].uid === agent.uid){
          agents[i] = agent;
          if (selectedAgent && selectedAgent.uid === agent.uid)
            selectedAgent = agent;
          break;
        }
      }
    });

    // Called when a new task is created.
    socket.on('new_task', (task) => {

      const agentID = task.agentID;
      if (!(agentID in tasks))
        tasks[agentID] = [];
      tasks[agentID].unshift(task);
      if (selectedAgent && selectedAgent.uid === agentID)
        selectedAgentTasks = tasks[agentID];
    });

    // Called when a task is updated.
    socket.on('update_task', (task) => {

      const agentID = task.agentID;
      const agentTasks = tasks[agentID];
      for (let i = 0; i < agentTasks.length; i++){
        if (agentTasks[i] === undefined)
          continue;
        if (agentTasks[i].uid === task.uid){
          agentTasks[i] = task;
          if (selectedAgent && selectedAgent.uid === agentID)
            selectedAgentTasks = agentTasks;
          break;
        }
      }
    });

    // Called when a task gets deleted.
    socket.on('task_deleted', (data) => {
      removeTask(data.agentID, data.taskID);
    });

    // Called when an agents gets deleted.
    socket.on('agent_deleted', (data) => {
      removeAgent(data.agentID)
    });

    // For writing some text to the console of an agent.
    socket.on('agent_console_output', (data) => {
      updateConsoleMessage(data.agentID, data.msg);
    });

    // Called when a new team chat message was created.
    socket.on('new_teamchat_message', (message) => {
      teamchatMessages = [...teamchatMessages, message];
    });

    // Called when new user is created.
    socket.on("new_user", (user) => {
      users = [...users, user];
    });

    // Called when a user gets deleted.
    socket.on("user_deleted", (username) => {

      if (selectedUser && selectedUser.username === username)
        releaseUser();
      users = users.filter(user => user.username !== username);
    });

    // Called when a user is updated.
    socket.on("user_updated", (user) => {
      
      if (selectedUser && selectedUser.username === user.username)
        selectedUser = user;
      for (let i = 0; i < users.length; i++){
        if (users[i].username === user.username){
          users[i] = user;
          break;
        }
      }
    });

    // Called when a new auth key was added.
    socket.on("new_authkey", (newKey) => {
      authKeys = [newKey, ...authKeys];
    });

    // Called when an auth key was deleted.
    socket.on("authkey_deleted", (key) => {

      if (selectedAuthKey && selectedAuthKey.key === key)
        releaseAuthKey();
      authKeys = authKeys.filter(k => k.key !== key);
    });

    // Called when an auth key was updated.
    socket.on("authkey_updated", (key) => {

      if (selectedAuthKey && selectedAuthKey.key === key.key)
        selectedAuthKey = key;
      for (let i = 0; i < authKeys.length; i++){
        if (authKeys[i].key ===  key.key){
          authKeys[i] = key;
          break;
        }
      }
    });

    // Called when a new redirector was added.
    socket.on("new_redirector", (rd) => {
      redirectors = [rd, ...redirectors];
    });

    // Called when a redirector was deleted.
    socket.on("redirector_deleted", (id) => {
      redirectors = redirectors.filter(r => r._id !== id);
    });

    // Handles custom errors.
    socket.on('striker_error', (err) => {
      console.log("Striker Error: " + err);
    });

    // Handles server-side disconnection.
    socket.on('disconnect', () => {
      console.log("Web socket disconnected!");
    });
  };

  // Load agents from the server using the REST API.
  const loadAgents = async () => {

    try{
      const res = await fetch(`${session.api}/agent`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      agents = data;
      for (let i = 0; i < agents.length; i++)
        consoleMsgs[agents[i].uid] = [];
    }catch(err){
      alert("Error loading agents: " + err.message);
    }
  };

  // Load all tasks from the server using the REST API.
  const loadTasks = async () => {

    try{
      const res = await fetch(`${session.api}/agent/tasks`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      tasks = data;
    }catch(err){
      alert("Error loading tasks: " + err.message);
    }
  };

  // Load all team chat messages from the server using the REST API.
  const loadTeamchatMessages = async () => {

    try{
      const res = await fetch(`${session.api}/chat/messages`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      teamchatMessages = data;
    }catch(err){
      alert("Error loadng team chat: " + err.message);
    }
  };

  // Load all users (for admins only)
  const loadUsers = async () => {

    try{
      let res = await fetch(`${session.api}/user`, {
        credentials: "include"
      });
      let data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      users = data;
    }catch(err){
      alert("Error loading users: " + err.message);
    }
  };

  // Load all agent auth keys.
  const loadAuthKeys = async () => {

    try{
      const res = await fetch(`${session.api}/key`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      authKeys = data;
    }catch(err){
      alert("Error loading keys: " + err.message);
    }
  };

  // Load all redirectors.
  const loadRedirectors = async () => {

    try{
      const res = await fetch(`${session.api}/redirector`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      redirectors = data;
    }catch(err){
      alert("Error loading redirectors: " + err.message);
    }
  };

  // Remove an agent from local cache. Called after an agent has been deleted.
  const removeAgent = (agentID) => {

    for (let i = 0; i < agents.length; i++){
      if (agents[i].uid === agentID){
        agents.splice(i, 1);
        tasks[agentID] = [];
        if (selectedAgent && selectedAgent.uid === agentID){
          selectedAgent = null;
          selectedAgentTasks = null;
          if (session.page === "agentPage")
            switchPage("agents");
        }
        return;
      }
    }
  };

  // Remove a task from local cache. Called after a task has been deleted.
  const removeTask = (agentID, taskID) => {

    if (tasks[agentID] === undefined)
      return;
    for (let i = 0; i < tasks[agentID].length; i++){
      if (tasks[agentID][i].uid === taskID){
        tasks[agentID].splice(i, 1); // remove it.
        if (selectedAgent && selectedAgent.uid === agentID) // apply changes to selected task if it's the target.
          selectedAgentTasks = tasks[selectedAgent.uid];
        return;
      }
    }
  };

  // Handles the agent selection event generated by AgentList.svelte
  const useAgent = (e) => {

    selectedAgent = e.detail;
    selectedAgentTasks = tasks[selectedAgent.uid];
    switchPage("agentPage");
  };

  // Update the console message of a agent.
  const updateConsoleMessage = (agentID, msg) => {

    // Remove trailing newlines.
    msg = msg.replace(/\r\n+$/, "");
    msg = msg.replace(/\n+$/, "");
    if (!(agentID in consoleMsgs))
      consoleMsgs[agentID] = [];
    consoleMsgs[agentID] = [...consoleMsgs[agentID], msg];
  };

  /**
   * Handles the 'updateConsole' event generated by child components for updating the console text of an agent.
   * This should not be used often since the server should be the one controlling console data.
   */
  const updateConsoleHandler = (e) => {
    updateConsoleMessage(e.detail.agentID, e.detail.msg);
  };

  // Clear the console text of an agent.
  const clearConsole = (agentID) => {
    consoleMsgs[agentID] = [];
  };

  // Handles the 'clearConsole' event generated by child components for clearing console text of an agent.
  const clearConsoleHandler = (e) => {
    clearConsole(e.detail);
  };

  // Handles the `sendMessage` event created by TeamChat.svelte
  const sendTeamchatMessage = (e) => {
    socket.emit("send_teamchat_message", e.detail);
  };

  // Handles the `selectUser` event created by Users.svelte
  const selectUser = (e) => {
    selectedUser = users[e.detail];
    showUserModal = true;
  };

  // Handles the `releaseUser` event created by Users.svelte
  const releaseUser = () => {
    showUserModal = false;
    selectedUser = null;
  };

  // Handles the `selectAuthKey` event created by AuthKeys.svelte
  const selectAuthKey = (e) => {
    selectedAuthKey = authKeys[e.detail];
    showSelectedKeyModal = true;
  };

  // Handles the `releaseAuthKey` event created by AuthKeys.svelte
  const releaseAuthKey = () => {
    showSelectedKeyModal = false;
    selectedAuthKey = null;
  };

  let agents = []; // All available agents.
  let selectedAgent = null; // The current agent being handled by the user
  let tasks = {}; // All agent IDs mapped to an array of their tasks.
  let selectedAgentTasks = null; // Tasks of the current agent being handled
  let consoleMsgs = {}; // IDs of agents mapped to the list of texts to display in their console view.
  let teamchatMessages = []; // Team server chat messeges.
  let users = []; // All available users, for admins only.
  let selectedUser = null; // The currently selected user.
  let showUserModal = false; // Controls the user management modal of Users.svelte
  let authKeys = []; // Authentication keys for agents.
  let selectedAuthKey = null; // The selected auth key.
  let showSelectedKeyModal = false; // Decides when the modal for a selected key is shown.
  let redirectors = []; // All available redirectors.

  let socket = null;
  let session = loadSession();
  if (session.loggedIn)
    setupC2(); // Reload C2 data since we already logged in.

  if (["agentPage"].indexOf(session.page) !== -1){ // Pages we shouldn't resume a session in
    if (session.loggedIn)
      session.page = "agents";
    else
      session.page = "login";
  }

  if (session.page === "login" || session.page === "register"){
    localStorage.removeItem("striker");
    session = createSession();
  }

</script>

<div>
  {#if (session.page === "login")}
    <Login {session} on:switchPage={switchPageHandler} on:loggedIn={loggedIn}/>
  {:else if (session.page === "register")}
    <Register {session} on:switchPage={switchPageHandler}/>
  {:else}
    <div class="main-body no-scrollbar w-full grid grid-cols-6">
      <!-- Header -->
      <div class="header bg-gray-900 italic text-white col-span-6 border-b-2">
        <p class="text-4xl text-center h-full mt-5 text-green-500"><Fa icon={icons.faExplosion} class="inline-block w-10 text-red-700"/> Striker <span class="text-red-700">C2</span></p>
      </div>

      <!-- The side nav -->
      <div class="page-nav bg-gray-900 border-gray-300 text-gray-300 col-span-1 pl-5 pt-10 shadow-md shadow-black">
        <ul class="main-nav">
          <li class={session.page === "agents" ? "text-green-600" : ""} on:click={() => switchPage("agents")}><Fa icon={icons.faRobot} class="inline-block w-10"/>Agents</li>
          <li class={session.page === "redirectors" ? "text-green-600" : ""} on:click={() => switchPage("redirectors")}><Fa icon={icons.faArrowsSpin} class="inline-block w-10"/>Redirectors</li>
          <li class={session.page === "keys" ? "text-green-600" : ""} on:click={() => switchPage("keys")}><Fa icon={icons.faKey} class="inline-block w-10"/>Auth Keys</li>
          <li class={session.page === "chat" ? "text-green-600" : ""} on:click={() => switchPage("chat")}><Fa icon={icons.faMessage} class="inline-block w-10"/>Team Chat</li>
          <!-- Addutional menu for administrators -->
          {#if (session.admin)}
            <li class={session.page === "users" ? "text-green-600" : ""} on:click={() => switchPage("users")}><Fa icon={icons.faUsers} class="inline-block w-10"/>Users</li>
            <li class={session.page === "logs" ? "text-green-600" : ""} on:click={() => switchPage("logs")}><Fa icon={icons.faMicroscope} class="inline-block w-10"/>Logs</li>
          {/if}
          <li on:click={logout}><Fa icon={icons.faDoorOpen} class="inline-block w-10"/>Logout</li>
        </ul>
      </div>

      <!-- The current main page -->
      <div class="page-body col-span-5 no-scrollbar overflow-y-auto px-5 py-5 pt-10 bg-gray">

        {#if (session.page === "agents")}
          <!-- List agents -->
          <AgentsList {agents} {tasks} on:selectedAgent={useAgent}/>
        {:else if (session.page === "agentPage" && selectedAgent !== null)}
          <AgentHandler {session} {socket} agent={selectedAgent} tasks={selectedAgentTasks} consoleMsgs={consoleMsgs[selectedAgent.uid]} on:clearConsole={clearConsoleHandler}/>
        {:else if (session.page === "keys")}
          <AuthKeys {session} {authKeys} {selectedAuthKey} {showSelectedKeyModal} on:selectAuthKey={selectAuthKey} on:releaseAuthKey={releaseAuthKey}/>
        {:else if (session.page === "chat")}
          <TeamChat messages={teamchatMessages} on:sendMessage={sendTeamchatMessage}/>
        {:else if (session.page === "users")}
          <Users {session} {users} {selectedUser} {showUserModal} on:selectUser={selectUser} on:releaseUser={releaseUser}/>
        {:else if (session.page === "redirectors")}
          <Redirectors {session} {redirectors}/>
        {:else}
          <ErrorMsg error={`Invalid page: ${session.page}`}/>
        {/if}

      </div>

    </div>
  {/if}
</div>

<style>

  :root{
    --header-height: 4em;
  }

  .header{
    height: var(--header-height);
  }

  .page-nav, .page-body{
    height: calc(100vh - var(--header-height));
  }

</style>
