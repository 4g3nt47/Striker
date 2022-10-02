<script>
  
  import {onMount} from 'svelte';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import Button from '../Button.svelte';
  import SuccessMsg from '../SuccessMsg.svelte';
  import ErrorMsg from '../ErrorMsg.svelte';

  export let session = {};
  export let agent = {};

  let files = [];
  let loading = true;
  let loadError = "", uploadError = "", uploadSuccess = "";
  let uploadBtn = null;

  /**
   * Load files from the server. Called on component mount and after every upload.
   */
  const loadFiles = async () => {

    loading = true;
    try{
      const rsp = await fetch(`${session.api}/agent/files/${agent.uid}`, {
        credentials: "include"
      });
      const data = await rsp.json();
      if (rsp.status != 200)
        throw new Error(data.error);
      files = data.reverse();
    }catch(err){
      loadError = err.message;
    }finally{
      loading = false;
    }
  };

  /**
   * Perform the file upload.
   */
  const uploadFile = async () => {

    uploadSuccess = "";
    uploadError = "";
    uploadBtn.innerText = "Uploading...";
    uploadBtn.disabled = true;
    try{  
      const formData = new FormData();
      const file = document.getElementById("file").files[0];
      if (!file)
        throw new Error("Please select a file to upload!");
      formData.append("file", file);
      const rsp = await fetch(`${session.api}/agent/upload/${agent.uid}`, {
        credentials: "include",
        method: "POST",
        body: formData
      });
      const data = await rsp.json();
      if (rsp.status !== 200)
        throw new Error(rsp.error);
      uploadSuccess = data.success;
      loadFiles();
    }catch(err){
      uploadError = err.message;
    }finally{
      uploadBtn.disabled = false;
      uploadBtn.innerText = "Upload File";
    }
  };

  // Download a file. Called when a file was clicked on.
  const download = (file) => {
    window.open(`${session.api}/agent/download/${file.uid}`);
  };

  onMount(loadFiles);

</script>

<div class="no-scrollbar border-2 border-black p-2">
  <div class="text-center">
    <div>
      <label for="file">Upload file:</label>
      <input id="file" type="file">
    </div>
    <div class="w-1/4 mx-auto mt-2">
      <Button type="secondary" bind:btn={uploadBtn} on:click={uploadFile}>Upload File</Button>
    </div>
  </div>
  <SuccessMsg success={uploadSuccess}/>
  <ErrorMsg error={uploadError}/>
  <div class="mt-3">
    {#if (loading)}
      <p>Loading downloaded files, please wait...</p>
    {:else if (files.length == 0)}
      {#if (loadError.length > 0)}
        <ErrorMsg error={loadError}/>
      {:else}
        <ErrorMsg error="No files available for this agent!"/>
      {/if}
    {:else}
      <!-- List the files  -->
      <div class="w-full no-scrollbar overflow-auto">
        <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
          <tr class="bg-gray-900 text-white">
            <th><Fa icon={icons.faIdCard} class="inline-block w-10 text-cyan-400"/>File ID</th>
            <th><Fa icon={icons.faCalendar} class="inline-block w-10 text-purple-500"/>Date Created</th>
            <th><Fa icon={icons.faPen} class="inline-block w-10 text-amber-400"/>Filename</th>
          </tr>
          {#each files as file, index}
            <tr class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" on:click={() => download(file)}>
              <td class="pl-2">{file.uid.substr(0, 12) + '...'}</td>
              <td class="pl-2">{new Date(file.dateCreated).toLocaleString()}</td>
              <td class="pl-2">{file.name}</td>
            </tr>
          {/each}
        </table>        
      </div>
    {/if}
  </div>
</div>
