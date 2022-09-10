<script>
  
  /**
   * @file This component displays a list of all available tasks for an agent.
   * @author Umar Abdul
   */

  import {createEventDispatcher} from 'svelte';
  import {slide} from 'svelte/transition';
  import ErrorMsg from '../ErrorMsg.svelte';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import Modal from '../Modal.svelte';

  export let tasks = [];
  let showTaskModal = false;
  let selectedTask = null;
  let selectedTaskData = "";

  // Select a task. Called after a listed task is clicked.
  const selectTask = (task) => {
    
    selectedTask = task;
    selectedTaskData = "";
    const keys = selectedTask
    for (let k of Object.keys(selectedTask.data))
      selectedTaskData += `${k} => ${selectedTask.data[k]}\n`;
    selectedTaskData = selectedTaskData.trim();
    showTaskModal = true;
  };

  // Release the selected task. Called when the displayed modal is closed.
  const releaseTask = () => {
    
    selectedTask = null;
    showTaskModal = false;
  };

</script>

<!-- Show task modal if a task is selected -->
{#if (selectedTask !== null)}
  <Modal show={showTaskModal} on:modalClosed={releaseTask}>
    <div class="font-mono text-md">
      <!-- Info table -->
      <table class="w-full">
        <tr>
          <th class="w-1/3 text-right pr-2">Task ID:</th>
          <td>{selectedTask.uid}</td>
        </tr>
        <tr>
          <th class="w-1/3 text-right pr-2">Task type:</th>
          <td>{selectedTask.taskType}</td>
        </tr>
        <tr>
          <th class="w-1/3 text-right pr-2">Owner:</th>
          <td>{selectedTask.owner}</td>
        </tr>
        <tr>
          <th class="w-1/3 text-right pr-2">Received:</th>
          <td class={selectedTask.received ? "text-blue-700" : ""}>{selectedTask.received ? "yes" : "no"}</td>
        </tr>
        {#if (selectedTask.completed)}
          <tr>
            <th class="w-1/3 text-right pr-2">Date Completed:</th>
            <td>{new Date(selectedTask.dateCompleted).toLocaleString()}</td>
          </tr>
        {/if}
      </table>
      <!-- Task Data and result -->
      <p>Task Data:</p>
      <div class="no-scrollbar max-h-80 overflow-y-auto p-1 text-white break-all bg-gray-900 border-2 border-black">
        {selectedTaskData}
      </div>
      {#if (selectedTask.completed)}
        <p>Task Result:</p>
        <div class="no-scrollbar max-h-80 overflow-y-auto break-all p-1 text-white bg-gray-900 border-2 border-black">
          {selectedTask.result.length > 0 ? selectedTask.result : "[Task returned no result]"}
        </div>
      {/if}
      <!-- Todo: Task management options -->
    </div>
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
      <tr transition:slide|local={{duration: 200}} class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" on:click={() => selectTask(task)}>
        <td class="pl-2">{task.uid}</td>
        <td class="pl-2">{task.taskType}</td>
        <td class="pl-2">{task.owner}</td>
        <td class={`pl-2 font-bold ${task.received ? 'text-blue-700' : ''}`}>{task.received ? "yes" : "no"}</td>
        <td class={`pl-2 font-bold ${task.completed ? 'text-green-700' : ''}`}>{task.completed ? "yes" : "no"}</td>
      </tr>
    {/each}
  </table>
{/if}
