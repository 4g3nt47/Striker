<script>
  
  import {onMount} from 'svelte';
  import Button from '../Button.svelte';
  import SuccessMsg from '../SuccessMsg.svelte';
  import ErrorMsg from '../ErrorMsg.svelte';

  export let session = {};
  export let agent = {};

  let files = [];
  let loading = true;
  let loadError = "";

  onMount(async () => {

    try{
      const res = await fetch(`${session.api}/agent/files/${agent.uid}`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status != 200)
        throw new Error(data.error);
      files = data;
    }catch(err){
      loadError = err.message;
    }finally{
      loading = false;
    }
  });

</script>

<div class="border-2 border-black p-2">
  <div class="text-center">
    <div>
      <label for="file">Upload file:</label>
      <input id="file" type="file">
    </div>
    <div class="w-1/4 mx-auto mt-2">
      <Button type="secondary">Upload File</Button>
    </div>
  </div>
  <div class="mt-3">
    {#if (loading)}
      <p>Loading downloaded files, please wait...</p>
    {:else if (files.length == 0)}
      {#if (loadError.length > 0)}
        <ErrorMsg error={loadError}/>
      {:else}
        <ErrorMsg error="No files for this agent!"/>
      {/if}
    {:else}
      <!-- List the files  -->
      <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
        <tr class="bg-gray-900 text-white">
          <th>File ID</th>
          <th>Filename</th>
          <th>Date Created</th>
        </tr>
        {#each files as file, index}
          <tr class="border-b-2 border-gray-900">
            <td>{file.uid}</td>
            <td>{file.name}</td>
            <td>{new Date(file.dateCreated).toLocaleString()}</td>
          </tr>
        {/each}
      </table>
    {/if}
  </div>
</div>
