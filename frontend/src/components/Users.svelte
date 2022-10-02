<script>
  
  /**
   * @file User management UI for admins.
   * @author Umar Abdul
   */

  import {onMount} from 'svelte';
  import Button from './Button.svelte';
  import Modal from './Modal.svelte';
  import SuccessMsg from './SuccessMsg.svelte';
  import ErrorMsg from './ErrorMsg.svelte';
  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';

  export let session = {};
  let users = {};
  let selectedUser = null;
  let showUserModal = false;
  let loading = true;
  let loadError = "";

  // Fetch users from the backend.
  const loadUsers = async () => {

    try{
      let res = await fetch(`${session.api}/user`, {
        credentials: "include"
      });
      let data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      users = data;
      console.log(users);
    }catch(err){
      loadError = err.message;
    }
    loading = false;
    selectUser(0);
  };

  const selectUser = (id) => {
    selectedUser = users[id];
    showUserModal = true;
  };

  const releaseUser = () => {
    showUserModal = false;
    selectedUser = null;
  };

  onMount(loadUsers);

</script>




<div>
  {#if (loading)}
    <p>Loading users, please wait...</p>
  {:else if (loadError)}
    <ErrorMsg error={loadError}/>
  {:else}
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
          <div class="mt-5 w-full grid grid-cols-3 gap-2">
            <Button type="custom" custom="text-white bg-gray-900 border-gray-900">Grant Admin</Button>
            <Button type="custom" custom="text-white bg-gray-900 border-gray-900">Suspend User</Button>
            <Button type="danger">Delete User</Button>
          </div>
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
        <tr transition:slide|local={{duration: 200}} class="cursor-pointer hover:bg-gray-900 hover:text-white duration-75 border-b-2 border-gray-900" on:click={() => selectUser(index)}>
          <td class="pl-2">{user.username}</td>
          <td class="pl-2">{new Date(user.creationDate).toLocaleString()}</td>
          <td class="pl-2">{new Date(user.lastSeen).toLocaleString()}</td>
          <td class={"pl-2 font-bold" + (user.admin ? " text-red-500" : "")}>{user.admin ? "Yes" : "No"}</td>
        </tr>
      {/each}
    </table>
  {/if}
</div>