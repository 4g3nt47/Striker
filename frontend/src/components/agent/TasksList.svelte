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

  export let tasks = [];

</script>

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
      <tr transition:slide|local={{duration: 200}} class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900">
        <td class="pl-2">{task.uid}</td>
        <td class="pl-2">{task.taskType}</td>
        <td class="pl-2">{task.owner}</td>
        <td class="pl-2">{task.received}</td>
        <td class="pl-2">{task.completed}</td>
      </tr>
    {/each}
  </table>
{/if}