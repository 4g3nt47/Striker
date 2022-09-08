<script>
  
  /**
   * @file This component is used to list display a list of all agents.
   * @author Umar Abdul
   */
  
  import {createEventDispatcher} from 'svelte';
  import {scale, slide, fade} from 'svelte/transition';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import ErrorMsg from '../ErrorMsg.svelte';

  export let session = {};
  export let agents = {};

  const dispatch = createEventDispatcher();

</script>

{#if (agents.length === 0)}
  <ErrorMsg error="No agents available at the moment!"/>
{:else}
  <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
    <tr class="bg-gray-900 text-white">
      <th><Fa icon={icons.faRobot} class="inline-block w-10 text-cyan-400"/>Agent ID</th>
      <th><Fa icon={icons.faUser} class="inline-block w-10 text-amber-600"/>User/Host</th>
      <th><Fa icon={icons.faDesktop} class="inline-block w-10 text-purple-400"/>OS</th>
      <th><Fa icon={icons.faBook} class="inline-block w-10 text-blue-500"/>Created</th>
      <th><Fa icon={icons.faEye} class="inline-block w-10 text-green-500"/>Last Callback</th>
    </tr>
    {#each agents as agent, index}
      <tr transition:slide|local={{duration: 200}} class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900">
        <td class="pl-2">{agent.uid}</td>
        <td class="pl-2">{`${agent.user}@${agent.host}`}</td>
        <td class="pl-2">{agent.os}</td>
        <td class="pl-2">{new Date().toLocaleString()}</td>
        <td class="pl-2">{new Date().toLocaleString()}</td>
      </tr>
    {/each}
  </table>
{/if}