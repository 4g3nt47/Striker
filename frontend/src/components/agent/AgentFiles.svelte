<script>
  
  import {onMount} from 'svelte';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import Button from '../Button.svelte';
  import SuccessMsg from '../SuccessMsg.svelte';
  import ErrorMsg from '../ErrorMsg.svelte';
  import Modal from '../Modal.svelte';

  export let session = {};
  export let socket = null;
  export let agent = {};

  let files = [];
  let selectedFile = null;
  let showFileModal = false;
  let loading = true;
  let loadError = "", uploadError = "", uploadSuccess = "";
  let modalSuccess = "", modalError = "";
  let uploadBtn = null;

  /**
   * Load files from the server. Called on component mount.
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
    }catch(err){
      uploadError = err.message;
    }finally{
      uploadBtn.disabled = false;
      uploadBtn.innerText = "Upload File";
    }
  };

  // Called when a file is clicked.
  const selectFile = (file) => {

    selectedFile = file;
    showFileModal = true;
  };

  // Called when selected file should be released.
  const releaseFile = () => {

    showFileModal = false;
    selectedFile = null;
  };

  // Download selected file.
  const downloadFile = () => {
    window.open(`${session.api}/agent/download/${selectedFile.uid}`);
  };

  // Called when selected file is to be deleted.
  const deleteFile = async () => {

    modalError = "";
    modalSuccess = "";
    if (!confirm("Delete this file?"))
      return;
    try{
      const res = await fetch(`${session.api}/agent/file/${agent.uid}/${selectedFile.uid}`, {
        credentials: "include",
        method: "DELETE"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
    }catch(err){
      modalError = err.message;
    }
  };

  // Called when selected file should be re-uploaded to the agent.
  const reuploadFile = async () => {

    modalError = "";
    modalSuccess = "";
    try{
      const res = await fetch(`${session.api}/agent/upload/${agent.uid}/${selectedFile.uid}`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      modalSuccess = data.success;
    }catch(err){
      modalError = err.message;
    }
  };

  // Called when a new file is uploaded.
  socket.on("new_file", (file) => {
    files = [file, ...files];
  });

  // Called when an existing file is updated.
  socket.on("file_updated", (file) => {

    for (let i = 0; i < files; i++){
      if (file.uid === files[i].uid){
        files[i] = file;
        break;
      }
    }
    if (selectedFile && selectedFile.uid === file.uid)
      selectedFile = file;
  });

  // Called when a file is deleted.
  socket.on("file_deleted", (fileID) => {

    if (selectedFile && selectedFile.uid === fileID)
      releaseFile();
    files = files.filter(f => f.uid !== fileID);
  });

  onMount(loadFiles);

</script>

{#if (selectedFile)}
  <Modal width="w-1/2" show={showFileModal} on:modalClosed={releaseFile}>
    <div>      
      <table class="w-full">
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">File ID</th>
          <td class="pl-2">{selectedFile.uid}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Filename</th>
          <td class="pl-2">{selectedFile.name}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">File Size</th>
          <td class="pl-2">{`${((selectedFile.size) / (1024 * 1024)).toFixed(3)} MB`}</td>
        </tr>        
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Downloads Count</th>
          <td class="pl-2">{selectedFile.downloadsCount}</td>
        </tr>
        <tr class="border-2 border-gray-900">
          <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Date Created</th>
          <td class="pl-2">{new Date(selectedFile.dateCreated).toLocaleString()}</td>
        </tr>
      </table>
      <div class="w-full my-3 grid grid-cols-3 gap-3">
        <Button on:click={downloadFile}>Download</Button>
        <Button on:click={reuploadFile}>Re-upload</Button>
        <Button type="danger" on:click={deleteFile}>Delete</Button>
      </div>
      <SuccessMsg success={modalSuccess}/>
      <ErrorMsg error={modalError}/>
    </div>
  </Modal>
{/if}

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
            <tr class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" on:click={() => selectFile(file)}>
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
