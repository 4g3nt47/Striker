<script>

  import {createEventDispatcher} from 'svelte';
  import {slide} from 'svelte/transition';
  import Button from './Button.svelte';
  import Modal from './Modal.svelte';
  import SuccessMsg from './SuccessMsg.svelte';
  import ErrorMsg from './ErrorMsg.svelte';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';

  export let session = {};
  export let socket = null;
  export let authKeys = [];
  export let selectedAuthKey = null;
  export let showSelectedKeyModal = false;

  const dispatch = createEventDispatcher();
  let showAddKeyModal = false;
  let inputKey = "";
  let inputKeyType = "0";
  let addKeyError = "";
  let selectedKeyModalError = "";

  const addKeyInit = () => {

    inputKey = "";
    addKeyError = "";
    for (let i = 0; i < 16; i++)
      inputKey += parseInt(Math.floor(Math.random() * 256)).toString(16).padStart(2, '0');
    showAddKeyModal = true;
  };

  const addKey = async () => {

    try{
      const res = await fetch(`${session.api}/key`, {
        method: 'POST',
        credentials: 'include',
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({
          key: inputKey,
          keyType: parseInt(inputKeyType)
        })
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      showAddKeyModal = false;
    }catch(err){
      addKeyError = err.message;
    }
  };

  const deleteKey = async () => {

    if (!confirm("Are you sure?"))
      return;
    selectedKeyModalError = "";
    try{
      const res = await fetch(`${session.api}/key/${selectedAuthKey.key}`, {
        method: 'DELETE',
        credentials: 'include'
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
    }catch(err){
      selectedKeyModalError = err.message;
    }
  };

  const selectAuthKey = (index) => {
    dispatch("selectAuthKey", index);
  };

  const releaseAuthKey = () => {
    selectedKeyModalError = "";
    dispatch("releaseAuthKey");
  };

</script>

<!-- New key modal -->
<Modal width="w-1/2" show={showAddKeyModal} on:modalClosed={() => showAddKeyModal = false}>
  <div>
    <form class="w-full" on:submit|preventDefault={addKey}>
      <label for="key">Key:</label>
      <div class="grid grid-cols-12 gap-2">
        <input class="col-span-11" type="text" bind:value={inputKey} placeholder="Key..." spellcheck="false" autocomplete="off" required>
        <span on:click={() => navigator.clipboard.writeText(inputKey)}><Fa icon={icons.faClipboard} class="inline text-gray-900 mt-2 hover:text-green-600"/></span>
      </div>
      <label for="key-type">Key Type:</label>
      <select id="key-type" class="mb-5 inline-block w-56" bind:value={inputKeyType}>
        <option value=0>Static</option>
        <option value=1>One time</option>
      </select>
      <Button btnType="submit">Add Key</Button>
      <ErrorMsg error={addKeyError}/>
    </form>
  </div>
</Modal>

<!-- Selected key modal -->
<Modal width="w-1/2" show={showSelectedKeyModal} on:modalClosed={releaseAuthKey}>
  {#if (selectedAuthKey)}
    <div class="font-mono text-md">
      <table class="w-full">
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Key</th>
          <td class="pl-2">{selectedAuthKey.key} <span on:click={() => navigator.clipboard.writeText(selectedAuthKey.key)}><Fa icon={icons.faClipboard} class="inline text-gray-900 mt-2 hover:text-green-600"/></span></td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Key Type</th>
          <td class="pl-2">{selectedAuthKey.keyType === 0 ? "Static" : "One time"}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Creator</th>
          <td class="pl-2">{selectedAuthKey.owner}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Usage Count</th>
          <td class="pl-2">{selectedAuthKey.useCount}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Date Created</th>
          <td class="pl-2">{new Date(selectedAuthKey.creationDate).toLocaleString()}</td>
        </tr>
      </table>
      <div class="w-1/2 mt-5 mx-auto">
        <Button type="danger" on:click={deleteKey}>Delete Key</Button>
      </div>
      <ErrorMsg error={selectedKeyModalError}/>
    </div>
  {/if}
</Modal>

<div>
  <Button type="secondary" on:click={addKeyInit}>Add Key</Button>
  {#if (authKeys.length === 0)}
    <ErrorMsg error="No keys available!"/>
  {:else}
    <div class="mt-5">
      <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
        <tr class="bg-gray-900 text-white">
          <th><Fa icon={icons.faFingerprint} class="inline-block w-10 text-purple-400"/>Key Type</th>
          <th><Fa icon={icons.faKey} class="inline-block w-10 text-green-400"/>Key</th>
          <th><Fa icon={icons.faClock} class="inline-block w-10 text-gray-400"/>Use Count</th>
          <th><Fa icon={icons.faCalendar} class="inline-block w-10 text-blue-400"/>Date</th>
        </tr>
        {#each authKeys as key, index}
          <tr transition:slide|local={{duration: 200}} class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" on:click={() => selectAuthKey(index)}>
            <td class="pl-2">{key.keyType === 0 ? "static" : "one time"}</td>
            <td class="pl-2">{key.key}</td>
            <td class="pl-3">{key.useCount}</td>
            <td class="pl-2">{new Date(key.creationDate).toLocaleString()}</td>
          </tr>
        {/each}
      </table>
    </div>
  {/if}
</div>
