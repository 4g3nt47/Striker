<script>
  
  /**
   * @file The primary component.
   * @author Umar Abdul
   */

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

  const switchPage = (e) => {
    session.page = e.detail;
  }

  let session = createSession();


</script>

<div>
  {#if (session.page === "login")}
    <Login {session} on:switchPage={switchPage}/>
  {:else if (session.page === "register")}
    <Register {session} on:switchPage={switchPage}/>
  {:else}
    <p>Others..</p>
  {/if}
</div>
