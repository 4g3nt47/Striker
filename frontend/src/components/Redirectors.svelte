<script>

  /**
   * @file Redirectors component.
   * @author Umar Abdul
   * Props:
   *        session - The session object.
   *        redirectors - An array of all available redirectors.
   */
  
  import {slide} from 'svelte/transition';
  import Button from './Button.svelte';
  import SuccessMsg from './SuccessMsg.svelte';
  import ErrorMsg from './ErrorMsg.svelte';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';

  export let session = {};
  export let redirectors = [];

  let newRedirector = "";
  let success = "", error = "";

  const addRedirector = async () => {
    
    success = "";
    error = "";
    const btn = document.getElementById("add-redirector");
    btn.innerText = "Adding...";
    btn.disabled = true;
    try{
      const res = await fetch(`${session.api}/redirector`, {
        credentials: "include",
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({
          url: newRedirector
        })
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      newRedirector = "";
      success = data.success;
    }catch(err){
      error = err.message;
    }finally{
      btn.disabled = false;
      btn.innerText = "Add Redirector";
    }
  };

  const deleteRedirector = async (id) => {
    
    if (!confirm("Are you sure?"))
      return;
    success = "";
    error = "";
    try{
      const res = await fetch(`${session.api}/redirector/${id}`, {
        credentials: "include",
        method: "DELETE"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      success = data.success;
    }catch(err){
      error = err.message;
    }
  };

</script>

<div class="mb-2">
  <form on:submit|preventDefault={addRedirector}>
    <div class="grid grid-cols-4 gap-2">
      <input class="col-span-3" type="text" placeholder="URL..." bind:value={newRedirector} required>
      <button id="add-redirector" class="bg-gray-900 text-white inline-block mt-1 leading-3 p-0 h-8 rounded hover:text-green-400 duration-100" type="submit">Add Redirector</button>
    </div>
    <SuccessMsg {success}/>
    <ErrorMsg {error}/>
  </form>  
</div>

<div>
  {#if (redirectors.length === 0)}
    <ErrorMsg error="No redirectors available at the moment!"/>
  {:else}
    <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
      <tr class="bg-gray-900 text-white">
        <th><Fa icon={icons.faListCheck} class="inline-block w-10 text-purple-400"/>S/N</th>
        <th><Fa icon={icons.faGlobe} class="inline-block w-10 text-green-400"/>URL</th>
        <th><Fa icon={icons.faHammer} class="inline-block w-10 text-red-700"/>Action</th>
      </tr>
      {#each redirectors as rd, index}
        <tr transition:slide|local={{duration: 200}} class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900">
          <td class="pl-2">{index + 1}</td>
          <td class="pl-2">{rd.url}</td>
          <td class="pl-7"><span on:click={() => deleteRedirector(rd._id)}><Fa icon={icons.faTrashCan} class="inline-block w-10 text-red-700"/></span></td>
      {/each}
    </table>
  {/if}
</div>
