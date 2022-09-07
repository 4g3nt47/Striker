<script>
  
  /**
   * @file The primary component.
   * @author Umar Abdul
   */

  import Fa from 'svelte-fa/src/fa.svelte';
  import * as icons from '@fortawesome/free-solid-svg-icons';
  import Login from './components/Login.svelte';
  import Register from './components/Register.svelte';

  const createSession = () => {
    return ({
      api: "http://localhost:3000",
      username: "",
      loggedIn: false,
      admin: false,
      page: "login"
    });
  };

  /**
   * Saves the session object to local storage.
   * @return {boolean} true on success.
   */
  const saveSession = () => {

    if (Storage){
      localStorage.setItem("striker", JSON.stringify(session));
      return true;
    }
    return false;
  };

  /**
   * Loads session from local storage, or create a new one if none found.
   * @return {object} The session object.
   */
  const loadSession = () => {
    
    let newSession = createSession();
    if (!Storage)
      return newSession;
    let savedSession = localStorage.getItem("striker");
    return (savedSession ? JSON.parse(savedSession) : newSession);
  };

  /**
   * Handles 'updateSession' event used by child components to update the session.
   * @param {obect} e - The event object.
   */
  const updateSession = (e) => {
    session = e.detail;
    saveSession();
  };

  const switchPage = (e) => {
    session.page = e.detail;
  }

  // Logout the user.
  const logout = () => {
    fetch(`${session.api}/user/logout`);
    session = createSession();
  };

  let session = loadSession();

</script>

<div>
  {#if (session.page === "login")}
    <Login {session} on:switchPage={switchPage} on:updateSession={updateSession}/>
  {:else if (session.page === "register")}
    <Register {session} on:switchPage={switchPage}/>
  {:else}
    <div class="main-body no-scrollbar w-full grid grid-cols-6">
      <!-- Header -->
      <div class="header bg-gray-900 italic text-white col-span-6 bg-gray-400">
        <p class="text-4xl text-center h-full mt-2 text-green-500">Striker <span class="text-red-700">C2</span></p>
      </div>
      <!-- The side nav -->
      <div class="page-nav bg-gray-900 border-t-2 border-gray-300 text-gray-300 col-span-1 pl-5 pt-10 shadow-md shadow-black">
        <ul class="main-nav">
          <li on:click={() => session.page = "agents"}><Fa icon={icons.faRobot} class="inline-block w-10" size="sm"/>Agents</li>
          <li on:click={() => session.page = "tasks"}><Fa icon={icons.faListCheck} class="inline-block w-10" size="sm"/>Tasks</li>
          <li on:click={() => session.page = "listeners"}><Fa icon={icons.faMicrophone} class="inline-block w-10" size="sm"/>Listeners</li>
          <li on:click={() => session.page = "redirectors"}><Fa icon={icons.faArrowsSpin} class="inline-block w-10" size="sm"/>Redirectors</li>
          <!-- Addutional menu for administrators -->
          {#if (session.admin)}
            <li on:click={() => session.page = "admin"}><Fa icon={icons.faCrown} class="inline-block w-10"/>Admin</li>
            <li on:click={() => session.page = "users"}><Fa icon={icons.faUsers} class="inline-block w-10" size="sm"/>Users</li>
            <li on:click={() => session.page = "logs"}><Fa icon={icons.faMicroscope} class="inline-block w-10" size="sm"/>Logs</li>
          {/if}
          <li on:click={logout}><Fa icon={icons.faDoorOpen} class="inline-block w-10" size="sm"/>Logout</li>
        </ul>
      </div>
      <!-- The current main page -->
      <div class="page-body col-span-5 overflow-y-auto pl-5 pt-5">
        
      </div>
    </div>
  {/if}
</div>

<style>
  
  :root{
    --header-height: 3em;
  }

  .header{
    height: var(--header-height);
  }

  .page-nav, .page-body{
    height: calc(100vh - var(--header-height));
  }


</style>