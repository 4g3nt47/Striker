<script>
  
  /**
   * @file Tasks list component.
   * @author Umar Abdul
   * Props:
   *          socket - The WS object.
   *          agent - The agent that owns the tasks.
   *          tasks - An array of tasks.
   *          showTaskModal - Controls modal visibility.
   *          selectedTask - The selected task.
   *          selectedTaskData - The data of selected task.
   * Events:
   *          selectTask - When selecting a task.
   *          releaseTask - When releasing a task.
   */

  import {createEventDispatcher} from 'svelte';
  import {slide} from 'svelte/transition';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import Button from '../Button.svelte';
  import ErrorMsg from '../ErrorMsg.svelte';
  import Modal from '../Modal.svelte';
  import {formatDate, formatDuration} from '../../lib/striker-utils.js';

  export let socket = null;
  export let agent = {};
  export let tasks = [];
  export let showTaskModal = false;
  export let selectedTask = null;
  export let selectedTaskData = "";
  export let killTaskBtn = null;

  const dispatch = createEventDispatcher();

  // Select a task. Called after a listed task is clicked.
  const selectTask = (task) => {
    dispatch("selectTask", task);
  };

  // Release the selected task. Called when the displayed modal is closed.
  const releaseTask = () => {
    killTaskBtn = null;
    dispatch("releaseTask");    
  };

  // Called when the delete task button of selected task is clicked.
  const deleteTask = () => {

    if (!confirm(`Delete task '${selectedTask.uid}'?`))
      return;
    socket.emit("agent_console_input", {
      agent,
      input: "delete task " + selectedTask.uid
    });
  };

  // Called when the kill task button of selected task is clicked.
  const killTask = () => {

    if (!confirm(`Kill task '${selectedTask.uid}'?`))
      return;
    socket.emit("agent_console_input", {
      agent,
      input: "kill " + selectedTask.uid
    });
    killTaskBtn.disabled = true;
  };

</script>

<!-- Show task modal if a task is selected -->
{#if (selectedTask !== null)}
  <Modal show={showTaskModal} on:modalClosed={releaseTask}>
    <div class="font-mono text-md">
      <!-- Info table -->
      <table class="w-full">
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Task ID</th>
          <td class="pl-2">{selectedTask.uid}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Task type</th>
          <td class="pl-2">{selectedTask.taskType}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Owner</th>
          <td class="pl-2">{selectedTask.owner}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Received</th>
          <td class="pl-2">{selectedTask.received ? formatDate(selectedTask.dateReceived) : "no"}</td>
        </tr>
        {#if (selectedTask && selectedTask.completed)}
          <tr class="border-2 border-gray-900">
            <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Date Completed</th>
            <td class="pl-2">{`${formatDate(selectedTask.dateCompleted)} (${formatDuration(selectedTask.dateCompleted - selectedTask.dateReceived)})`}</td>
          </tr>
        {/if}
      </table>
      <!-- Task Data and result -->
      <p>Task Data:</p>
      <div class="no-scrollbar max-h-80 overflow-y-auto p-1 text-white whitespace-pre-wrap break-all bg-gray-900 border-2 border-black">
        {selectedTaskData}
      </div>
      {#if (selectedTask && selectedTask.completed)}
        <p>Task Result:</p>
        <div class="no-scrollbar max-h-80 overflow-y-auto break-all whitespace-pre-wrap p-1 text-white bg-gray-900 border-2 border-black">
          {selectedTask.result.length > 0 ? selectedTask.result : "[Task returned no result]"}
        </div>
      {/if}
    </div>
    <!-- Task management options -->
    {#if (selectedTask && selectedTask.received && (!selectedTask.completed))}
      <div class="w-1/2 mt-2 mx-auto grid grid-cols-2 gap-2">
        <Button type="danger" bind:btn={killTaskBtn} on:click={killTask}>Kill Task</Button>
        <Button type="danger" on:click={deleteTask}>Delete Task</Button>        
      </div>
    {:else}
      <div class="w-1/3 mt-2 mx-auto">
        <Button type="danger" on:click={deleteTask}>Delete Task</Button>
      </div>
    {/if}
  </Modal>
{/if}

<!-- The tasks listing -->
{#if (tasks.length === 0)}
  <ErrorMsg error="No tasks available for this agent!"/>
{:else}
  <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
    <tr class="bg-gray-900 text-white">
      <th><Fa icon={icons.faIdCard} class="inline-block w-10 text-cyan-400"/>Task ID</th>
      <th><Fa icon={icons.faFingerprint} class="inline-block w-10 text-amber-400"/>Type</th>
      <th><Fa icon={icons.faUser} class="inline-block w-10 text-purple-400"/>Owner</th>
      <th><Fa icon={icons.faDownload} class="inline-block w-10 text-red-600"/>Received</th>
      <th><Fa icon={icons.faFlag} class="inline-block w-10 text-green-500"/>Completed</th>
    </tr>
    {#each tasks as task, index}
      <tr transition:slide|local={{duration: 200}} class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" title={JSON.stringify(task.data ? task.data : "{}").substr(0, 100)} on:click={() => selectTask(task)}>
        <td class="pl-2">{task.uid}</td>
        <td class="pl-2">{task.taskType}</td>
        <td class="pl-2">{task.owner}</td>
        <td class={`pl-2 font-bold ${task.received || task.completed ? 'text-blue-700' : ''}`}>{task.received || task.completed ? "yes" : "no"}</td>
        <td class={`pl-2 font-bold ${task.completed ? 'text-green-700' : ''}`}>{task.completed ? "yes" : "no"}</td>
      </tr>
    {/each}
  </table>
{/if}
