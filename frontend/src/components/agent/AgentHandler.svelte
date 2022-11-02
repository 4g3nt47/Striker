<script>
  
  /**
   * @file This component handles the interaction of user with an active agent.
   * @author Umar Abdul
   * Props:
   *        session - The session object
   *        socket - The connected web socket in use
   *        agent - The agent being handled
   *        tasks - The tasks of the agent
   *        consoleMsgs - The messages to display in the agent console
   * Events:
   *        clearConsole - Notify parent to clear the console messages `consoleMsgs` of the agent
   */

  import {onMount, createEventDispatcher} from 'svelte';
  import {scale, slide, fade} from 'svelte/transition';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import TasksList from './TasksList.svelte';
  import Button from '../Button.svelte';
  import ErrorMsg from '../ErrorMsg.svelte';
  import AgentFiles from './AgentFiles.svelte';
  import {formatDate, formatDuration} from '../../lib/striker-utils.js';

  export let session = {};
  export let socket = null;
  export let agent = {};
  export let tasks = [];
  export let consoleMsgs = [];
  
  const dispatch = createEventDispatcher();
  let showTaskModal = false; // Show modal of selected task.
  let selectedTask = null; // The selected task in tasks list.
  let selectedTaskData = ""; // The data/args of a selected task.
  let tabs = ["Info", "Tasks", "Console", "Files"];
  let currTab = tabs[0];
  let consoleCommand = "";
  let msgCount = 0;
  let deleteAgentBtn = null;
  let infoPageError = "";
  let cmdHistory = {cmds: [], index: 0};
  let agentTypes = ["Native C", "Python", "Unknown"];

  // Handles the `selectTask` event created by TasksList.svelte
  const selectTask = (e) => {

    selectedTask = e.detail;
    selectedTaskData = "";
    if (!selectedTask.data){
      selectedTaskData = "null";
    }else{
      for (let k of Object.keys(selectedTask.data))
        selectedTaskData += `${k} => ${selectedTask.data[k]}\n`;
    }
    selectedTaskData = selectedTaskData.trim();
    showTaskModal = true;
  };

  // Handles the `releaseTask` event created by TasksList.svelte
  const releaseTask = () => {
    
    showTaskModal = false;
    selectedTask = null;
    selectedTaskData = "";
  };

  // Updates our console text. Called whenever `consoleMsgs` changes. 
  const updateConsole = (consoleMsgs) => {
    
    if (msgCount === consoleMsgs.length) // A hacky solution for an autoscroll bug when nothing changes
      return;
    msgCount = consoleMsgs.length;
    consoleToBottom();
  };

  // Scroll to the bottom of the console display.
  const consoleToBottom = () => {

    setTimeout(() => {
      let elem = document.getElementById('console-text');
      if (elem)
        elem.scrollTop = elem.scrollHeight;
    }, 100);
  };

  // Called when tasks have been updated.
  const updateTasks = (tasks) => {

    if (selectedTask){
      let valid = false;
      for (let i = 0; i < tasks.length; i++){
        const task = tasks[i];
        if (task.uid === selectedTask.uid){
          valid = true;
          selectedTask = task;
          selectedTaskData = "";
          for (let k of Object.keys(selectedTask.data))
            selectedTaskData += `${k} => ${selectedTask.data[k]}\n`;
          break;
        }
      }
      if (!valid) // Task no longer exist.
        releaseTask();
    }
  };

  // A reactive expression to render new console messages.
  $: updateConsole(consoleMsgs);

  // A reactive expression to for tasks update.
  $: updateTasks(tasks);

  // Handles tab switch.
  const switchTab = (newTab) => {

    infoPageError = "";
    currTab = newTab;
    if (currTab === "Console"){
      consoleToBottom();
      setTimeout(() => {
        let input = document.getElementById('console-input');
        if (input)
          input.focus();
        setConsoleSize();
      }, 70);
    }
  };

  const setConsoleSize = () => {

    let consoleElem = document.getElementById('console-text');
    let wrapper = document.getElementById('tabs-container');
    if (consoleElem && wrapper)
      consoleElem.style['height'] = `${wrapper.offsetHeight - 95}px`;
  };

  /**
   * Execute the command entered in the console.
   * Connected to the `keyup` event of the command input field.
   */
  const consoleExec = (e) => {

    // Use arrow keys to allow going to previous commands.
    if (e.key === 'ArrowUp' || e.keyCode === 38){
      if (cmdHistory.length === 0)
        return;
      let cmd = cmdHistory.cmds[cmdHistory.index];
      if (cmd === undefined)
        return;
      consoleCommand = cmd;
      if (cmdHistory.index > 0)
        cmdHistory.index--;
      return;
    }
    if (e.key === 'ArrowDown' || e.keyCode === 40){
      if (cmdHistory.length === 0)
        return;
      let cmd = cmdHistory.cmds[cmdHistory.index + 1];
      if (cmd === undefined)
        return;
      consoleCommand = cmd;
      cmdHistory.index += 1;
      return;
    }
    if (!(e.key === 'Enter' || e.keyCode === 13)) // User hit the enter key?
      return;
    if (consoleCommand.trim().length === 0)
      return;
    // Add command to history if it's not the last one in the queue.
    if (cmdHistory.cmds && cmdHistory.cmds[cmdHistory.cmds.length - 1] !== consoleCommand){    
      cmdHistory.cmds.push(consoleCommand);
      cmdHistory.index = cmdHistory.cmds.length - 1;
    }
    if (consoleCommand === "clear"){
      dispatch("clearConsole", agent.uid);
      msgCount = 0;
    }else{
      socket.emit("agent_console_input", {
        agent,
        input: consoleCommand
      });
    }
    consoleCommand = "";
  };

  /**
   * Called when a user clicks on the `Delete Agent` button in the info page.
   */
  const deleteAgent = async () => {

    if (!confirm(`Delete agent '${agent.uid}'?`))
      return;
    deleteAgentBtn.innerText = "Deleting agent...";
    deleteAgentBtn.disabled = true;
    try{
      const res = await fetch(`${session.api}/agent/${agent.uid}`, {
        credentials: "include",
        method: "DELETE"
      });
      const data = await res.json();
      if (res.status != 200){
        throw new Error(data.error);
      }
      // No need to restore button since successful delete will kick us back to agents list page.
    }catch(err){
      infoPageError = err.message;
      deleteAgentBtn.disabled = false;
      deleteAgentBtn.innerText = "Delete Agent";
    }
  };

  onMount(() => {
    setConsoleSize();
    window.onresize = setConsoleSize;
  }); 

</script>

<div id="tabs-container" class="border-2 border-gray-900 min-h-full max-h-full no-scrollbar overflow-y-auto overflow-x-hidden">

  <!-- Our tabs -->
  <ul class="list-none text-center bg-gray-900 text-white">
    {#each tabs as tab, index}
      <li class={"hover:text-green-500 " + `inline-block w-32 px-5 mx-10 cursor-pointer ${tab === currTab ? "text-gray-900 bg-gray-300" : ""}`} on:click={() => switchTab(tab)}>{tab}</li>
    {/each}
  </ul>

  <!-- A div to wrap all our tabs -->
  <div class="mt-3 mx-3 text-lg no-scrollbar overflow-x-auto">
    <!-- Render the current tab -->
    {#if (currTab === "Info")}
      <table class="w-full border-2 border-black text-left font-mono">
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">ID</th>
          <td class="pl-3">{agent.uid}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Type</th>
          <td class="pl-3">{agentTypes[agent.agentType]}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">OS</th>
          <td class="pl-3">{agent.os}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Host</th>
          <td class="pl-3">{agent.host}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">User</th>
          <td class="pl-3">{agent.user}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Process ID</th>
          <td class="pl-3">{agent.pid}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Working directory</th>
          <td class="pl-3">{agent.cwd}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Date Created</th>
          <td class="pl-3">{formatDate(agent.dateCreated)}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Last Contact</th>
          <td class="pl-3">{formatDate(agent.lastSeen) + ` (${formatDuration(Date.now() - agent.lastSeen)})`}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Callback Delay</th>
          <td class="pl-3">{formatDuration(agent.delay * 1000)}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="w-1/4 text-right pr-2 bg-gray-900 text-white">Frozen</th>
          <td class={"pl-3 " + (agent.frozen ? "text-cyan-700" : "")}>{agent.frozen ? "yes" : "no"}</td>
        </tr>        
      </table>
      <div class="w-1/5 my-2 mx-auto">
        <Button type="danger" bind:btn={deleteAgentBtn} on:click={deleteAgent}>Delete Agent</Button>
      </div>
      <ErrorMsg error={infoPageError}/>
    {:else if (currTab === "Tasks")}
      <TasksList {socket} {agent} {tasks} {selectedTask} {showTaskModal} {selectedTaskData} on:selectTask={selectTask} on:releaseTask={releaseTask}/>
    {:else if (currTab === "Console")}
      <div id="console-text" class="w-full no-scrollbar font-mono text-md bg-gray-900 border-2 border-green-700 p-1 text-white overflow-y-auto whitespace-pre-wrap break-all border-2">
        {#each consoleMsgs as msg}
          {#if (msg.prompt)}
            <p><span class="text-green-400">{"("}</span><span class="text-yellow-400">{msg.prompt.trim()}</span><span class="text-green-400">{") > "}</span>{msg.msg}</p>
          {:else}
            <p>{msg.msg}</p>
          {/if}
        {/each}
      </div>
      <input id="console-input" class="w-full border-2 px-2 font-mono bg-gray-300 placeholder-gray-500" type="text" placeholder="command..." spellcheck="false" bind:value={consoleCommand} on:keyup={consoleExec} autocomplete="off"> 
    {:else if (currTab === "Files")}
      <AgentFiles {session} {socket} {agent}/>
    {/if}
  </div>
</div>
