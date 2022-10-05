<script>
  
  /**
   * @file User management UI for admins.
   * @author Umar Abdul
   */

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
  export let users = [];
  export let selectedUser = null;
  export let showUserModal = false;
  
  const dispatch = createEventDispatcher();
  let modalSuccess = "", modalError = "", newPassword = "";  

  const clearMsgs = () => {

    modalError = "";
    modalSuccess = "";
  };

  const selectUser = (index) => {

    clearMsgs();
    dispatch("selectUser", index);
  };

  const releaseUser = () => {

    clearMsgs();
    newPassword = "";
    dispatch("releaseUser");
  };

  const toggleAdmin = async () => {

    clearMsgs();
    try{
      const res = await fetch(`${session.api}/user/admin/${selectedUser.admin ? "revoke" : "grant"}/${selectedUser.username}`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
    }catch(err){
      modalError = err.message;
    }
  };

  const toggleSuspend = async () => {

    clearMsgs();
    try{
      const res = await fetch(`${session.api}/user/${selectedUser.suspended ? "activate" : "suspend"}/${selectedUser.username}`, {
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
    }catch(err){
      modalError = err.message;
    }
  };

  const deleteUser = async () => {

    if (!confirm("Are you sure?"))
      return;
    clearMsgs();
    try{
      const res = await fetch(`${session.api}/user/${selectedUser.username}`, {
        method: "DELETE",
        credentials: "include"
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);      
    }catch(err){
      modalError = err.message;
    }
  };

  const newPasswordKeyup = async (e) => {

    if (!(e.key === 'Enter' || e.key === 13))
      return;
    if (!newPassword)
      return;
    clearMsgs();
    try{
      const res = await fetch(`${session.api}/user/password/${selectedUser.username}`, {
        method: "POST",
        credentials: "include",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({password: newPassword})
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      newPassword = "";
      modalSuccess = data.success;
    }catch(err){
      modalError = err.message;
    }
  };

</script>

<div>
  {#if (selectedUser)}
    <Modal width="w-1/2" show={showUserModal} on:modalClosed={releaseUser}>
      <div class="font-mono text-md">
         <table class="w-full">
          <tr class="border-2 border-gray-900">
            <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Username</th>
            <td class="pl-2">{selectedUser.username}</td>
          </tr>
          <tr class="border-2 border-gray-900">
            <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Date Created</th>
            <td class="pl-2">{new Date(selectedUser.creationDate).toLocaleString()}</td>
          </tr>
          <tr class="border-2 border-gray-900">
            <th class="w-1/3 text-right pr-2 bg-gray-900 text-white">Last Login</th>
            <td class="pl-2">{new Date(selectedUser.lastSeen).toLocaleString()}</td>
          </tr>
        </table>
        <div class="mt-2 w-full">
          <input class="col-span-2" type="password" bind:value={newPassword} placeholder="New password..." on:keyup={newPasswordKeyup}>
        </div>
        <div class="mt-2 w-full grid grid-cols-3 gap-2">
          <Button type="custom" custom="text-white bg-gray-900 border-gray-900" on:click={toggleAdmin}>{selectedUser.admin ? "Revoke Admin" : "Grant Admin"}</Button>
          <Button type="custom" custom="text-white bg-gray-900 border-gray-900" on:click={toggleSuspend}>{selectedUser.suspended ? "Activate User" : "Suspend User"}</Button>
          <Button type="danger" on:click={deleteUser}>Delete User</Button>
        </div>
        <SuccessMsg success={modalSuccess}/>
        <ErrorMsg error={modalError}/>
      </div>
    </Modal>
  {/if}
  <table class="border-2 border-gray-900 w-full font-mono text-left text-lg">
    <tr class="bg-gray-900 text-white">
      <th><Fa icon={icons.faUser} class="inline-block w-10 text-cyan-400"/>Username</th>
      <th><Fa icon={icons.faBaby} class="inline-block w-10 text-purple-400"/>Date Created</th>
      <th><Fa icon={icons.faTicket} class="inline-block w-10 text-green-400"/>Last Login</th>
      <th><Fa icon={icons.faCrown} class="inline-block w-10 text-red-500"/>Admin</th>
    </tr>
    {#each users as user, index}
      <tr transition:slide|local={{duration: 200}} class={"cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" + (user.suspended ? " text-cyan-700" : "")} on:click={() => selectUser(index)}>
        <td class="pl-2">{user.username}</td>
        <td class="pl-2">{new Date(user.creationDate).toLocaleString()}</td>
        <td class="pl-2">{new Date(user.lastSeen).toLocaleString()}</td>
        <td class={"pl-2 font-bold" + (user.admin ? " text-red-500" : "")}>{user.admin ? "Yes" : "No"}</td>
      </tr>
    {/each}
  </table>
</div>