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

  import {createEventDispatcher} from 'svelte';
  import {scale, slide, fade} from 'svelte/transition';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import TasksList from './TasksList.svelte';
  import Button from '../Button.svelte';
  import ErrorMsg from '../ErrorMsg.svelte';
  import AgentFiles from './AgentFiles.svelte';

  export let session = {};
  export let socket = null;
  export let agent = {};
  export let tasks = [];
  export let consoleMsgs = [];
  
  const dispatch = createEventDispatcher();
  let tabs = ["Info", "Tasks", "Console", "Files"];
  let currTab = tabs[0];
  let consoleCommand = "";
  let consoleText = "";
  let msgCount = 0;
  let deleteAgentBtn = null;
  let infoPageError = "";

  // Updates our console text. Called whenever `consoleMsgs` changes. 
  const updateConsole = (consoleMsgs) => {
    
    if (msgCount === consoleMsgs.length) // A hacky solution for an autoscroll bug when nothing changes
      return;
    msgCount = consoleMsgs.length;
    consoleText = consoleMsgs.join("\n");
    consoleToBottom();
  };

  // Scroll to the bottom of the console display.
  const consoleToBottom = () => {

    setTimeout(() => {
      let elem = document.getElementById('console-text');
      if (elem)
        elem.scrollTop = elem.scrollHeight;
    }, 70);
  };

  // A reactive expression to update render new console messages.
  $: updateConsole(consoleMsgs);

  // Handles tab switch.
  const switchTab = (newTab) => {

    infoPageError = "";
    currTab = newTab;
    if (currTab === "Console")
      consoleToBottom();
  };

  /**
   * Execute the command entered in the console.
   * Connected to the `keyup` event of the command input field.
   */
  const consoleExec = (e) => {

    if (!(e.key === 'Enter' || e.keyCode === 13)) // User hit the enter key?
      return;
    if (consoleCommand.trim().length === 0)
      return;
    if (consoleCommand === "clear"){
      dispatch("clearConsole", agent.uid);
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

</script>

<div class="border-2 border-gray-900 min-h-full max-h-full no-scrollbar overflow-y-auto overflow-x-hidden">

  <!-- Our tabs -->
  <ul class="list-none text-center bg-gray-900 text-white">
    {#each tabs as tab, index}
      <li class={`inline-block w-32 px-5 mx-10 cursor-pointer ${tab === currTab ? "text-green-300 bg-gray-300 text-gray-900" : ""}`} on:click={() => switchTab(tab)}>{tab}</li>
    {/each}
  </ul>

  <!-- A div to wrap all our tabs -->
  <div class="m-3 text-lg no-scrollbar overflow-x-auto">
    <!-- Render the current tab -->
    {#if (currTab === "Info")}
      <table class="w-full border-2 border-black text-left font-mono">
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">ID:</th>
          <td>{agent.uid}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">OS:</th>
          <td>{agent.os}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">Host:</th>
          <td>{agent.host}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">User:</th>
          <td>{agent.user}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">Process ID:</th>
          <td>{agent.pid}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">Working directory:</th>
          <td>{agent.cwd}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">Date Created:</th>
          <td>{new Date(agent.dateCreated).toLocaleString()}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">Last Contact:</th>
          <td>{new Date(agent.lastSeen).toLocaleString() + ` (${((Date.now() - agent.lastSeen) / (1000 * 60)).toFixed(2)} minutes)`}</td>
        </tr>
        <tr class="border-b-2 border-gray-900">
          <th class="text-right pr-5">Frozen:</th>
          <td class={agent.frozen ? "text-cyan-700" : ""}>{agent.frozen}</td>
        </tr>        
      </table>
      <div class="w-1/5 my-2 mx-auto">
        <Button type="danger" bind:btn={deleteAgentBtn} on:click={deleteAgent}>Delete Agent</Button>
      </div>
      <ErrorMsg error={infoPageError}/>
    {:else if (currTab === "Tasks")}
      <TasksList {socket} {tasks}/>
    {:else if (currTab === "Console")}
      <textarea id="console-text" class="w-full no-scrollbar font-mono text-md bg-gray-900 border-2 border-black p-1 text-white break-all" rows="15" bind:value={consoleText} readonly></textarea>
      <input class="w-full border-2 border-gray-900 px-2 font-mono bg-gray-300 placeholder-gray-500" type="text" placeholder="command..." spellcheck="false" bind:value={consoleCommand} on:keyup={consoleExec}> 
    {:else if (currTab === "Files")}
      <AgentFiles {session} {agent}/>
    {/if}
  </div>
</div>

