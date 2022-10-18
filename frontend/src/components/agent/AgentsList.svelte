<script>
  
  /**
   * @file Agents list component.
   * @author Umar Abdul
   * Props:
   *        agents - An array of all agents.
   *        tasks - An array of all tasks.
   * Events:
   *        selectedAgent - When selecting an agent.
   */
  
  import {createEventDispatcher} from 'svelte';
  import {slide} from 'svelte/transition';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import ErrorMsg from '../ErrorMsg.svelte';

  export let agents = [];
  export let tasks = {};

  const dispatch = createEventDispatcher();

  const useAgent = (agent) => {
    dispatch("selectedAgent", agent);
  };

</script>

{#if (agents.length === 0)}
  <ErrorMsg error="No agents available at the moment!"/>
{:else}
  <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
    <tr class="bg-gray-900 text-white">
      <th><Fa icon={icons.faIdCard} class="inline-block w-10 text-cyan-400"/>Agent ID</th>
      <th><Fa icon={icons.faDesktop} class="inline-block w-10 text-purple-400"/>OS</th>
      <th><Fa icon={icons.faUser} class="inline-block w-10 text-amber-600"/>User/Host</th>
      <th><Fa icon={icons.faListCheck} class="inline-block w-10 text-blue-600"/>Tasks</th>
      <th><Fa icon={icons.faEye} class="inline-block w-10 text-green-500"/>Last Callback</th>
    </tr>
    {#each agents as agent, index}
      <tr transition:slide|local={{duration: 200}} class={"cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" + (agent.frozen ? " text-cyan-700" : "")} on:click={() => useAgent(agent)}>
        <td class="pl-2">{agent.uid}</td>
        <td class="pl-2">{agent.os}</td>
        <td class="pl-2">{`${agent.user}@${agent.host}`}</td>
        <td class="pl-2">{(tasks[agent.uid] === undefined ? 0 : tasks[agent.uid].length)}</td>
        <td class="pl-2">{new Date(agent.lastSeen).toLocaleString()}</td>
      </tr>
    {/each}
  </table>
{/if}

