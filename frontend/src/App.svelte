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

  const createSession = () => {

    const backend = "localhost:3000";
    return ({
      backend: "localhost:3000",
      api: `http://${backend}`,
      ws: `ws://${backend}`,
      username: "",
      loggedIn: false,
      admin: false,
      token: "",
      page: "login"
    });
  };

  /**
   * Saves the session object to local storage.
   * @return {boolean} true on success.
   */
  const saveSession = () => {

    if (Storage){
      localStorage.setItem("striker", JSON.stringify(session));
      return true;
    }
    return false;
  };

  /**
   * Loads session from local storage, or create a new one if none found.
   * @return {object} The session object.
   */
  const loadSession = () => {
    
    let newSession = createSession();
    if (!Storage)
      return newSession;
    let savedSession = localStorage.getItem("striker");
    return (savedSession ? JSON.parse(savedSession) : newSession);
  };

  /**
   * Handles 'updateSession' event used by child components to update the session.
   * @param {obect} e - The event object.
   */
  const updateSession = (e) => {
    session = e.detail;
    saveSession();
  };

  const switchPage = (e) => {
    session.page = e.detail;
    saveSession();
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

  // Run some setups following authentication.
  const setupC2 = () => {

    wsInit(session.token);
    loadAgents();
    loadTasks();
  };

  // Logout the user.
  const logout = async () => {

    fetch(`${session.api}/user/logout`, {
      credentials: "include"
    });
    session = createSession();
    saveSession();
  };

  // Configure the web socket client and define event handlers.
  const wsInit = (token) => {

    // Connect and authenticate.
    socket = io(session.ws, {
      auth: {
        token
      }
    });

    // Handle auth/connection error.
    socket.on('connect_error', (err) => {
      console.log(err.message);
    });

    // Called when a new agent is created.
    socket.on('new_agent', (agent) => {
      agents = [agent, ...agents];
      tasks[agent.uid] = [];
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

    // Handles custom errors.
    socket.on('striker_error', (err) => {
      console.log("Error: " + err);
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
        alert("Error loading agents: " + data.error);
      agents = data;
    }catch(err){
      alert(err.message);
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
        alert("Error loading tasks: " + data.error);
      tasks = data;
    }catch(err){
      alert(err.message);
    }
  };

  // Handles the agent selection event generated by AgentList.svelte
  const useAgent = (e) => {

    selectedAgent = e.detail;
    selectedAgentTasks = tasks[selectedAgent.uid];
    session.page = "agentPage";
  };

  let agents = [];
  let tasks = {};
  let selectedAgent = null;
  let selectedAgentTasks = null;

  let socket = null;
  let session = loadSession();
  if (session.loggedIn)
    setupC2();

</script>

<div>
  {#if (session.page === "login")}
    <Login {session} on:switchPage={switchPage} on:loggedIn={loggedIn}/>
  {:else if (session.page === "register")}
    <Register {session} on:switchPage={switchPage}/>
  {:else}
    <div class="main-body no-scrollbar w-full grid grid-cols-6">
      <!-- Header -->
      <div class="header bg-gray-900 italic text-white col-span-6 border-b-2">
        <p class="text-4xl text-center h-full mt-5 text-green-500"><Fa icon={icons.faExplosion} class="inline-block w-10 text-red-700"/> Striker <span class="text-red-700">C2</span></p>
      </div>

      <!-- The side nav -->
      <div class="page-nav bg-gray-900 border-gray-300 text-gray-300 col-span-1 pl-5 pt-10 shadow-md shadow-black">
        <ul class="main-nav">
          <li on:click={() => session.page = "agents"}><Fa icon={icons.faRobot} class="inline-block w-10"/>Agents</li>
          <li on:click={() => session.page = "tasks"}><Fa icon={icons.faListCheck} class="inline-block w-10"/>Tasks</li>
          <li on:click={() => session.page = "listeners"}><Fa icon={icons.faMicrophone} class="inline-block w-10"/>Listeners</li>
          <li on:click={() => session.page = "redirectors"}><Fa icon={icons.faArrowsSpin} class="inline-block w-10"/>Redirectors</li>
          <!-- Addutional menu for administrators -->
          {#if (session.admin)}
            <li on:click={() => session.page = "admin"}><Fa icon={icons.faCrown} class="inline-block w-10"/>Admin</li>
            <li on:click={() => session.page = "users"}><Fa icon={icons.faUsers} class="inline-block w-10"/>Users</li>
            <li on:click={() => session.page = "logs"}><Fa icon={icons.faMicroscope} class="inline-block w-10"/>Logs</li>
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
          <AgentHandler {session} {socket} agent={selectedAgent} tasks={selectedAgentTasks}/>
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