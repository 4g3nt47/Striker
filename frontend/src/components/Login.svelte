<script>

  import {onMount, createEventDispatcher} from 'svelte';
  import Button from './Button.svelte';
  import SuccessMsg from './SuccessMsg.svelte';
  import ErrorMsg from './ErrorMsg.svelte';

  export let session = {};
  const dispatch = createEventDispatcher();
  const fields = {username: "", password: ""};
  let loginBtn = null;
  let success = "", error = "";

  const login = async () => {

    success = "";
    error = "";
    loginBtn.innerText = "Login in...";
    loginBtn.disabled = true;
    try{
      const res = await fetch(`${session.api}/user/login`, {
        method: "POST",
        credentials: "include",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify(fields),
      });
      const data = await res.json();
      if (res.status !== 200)
        throw new Error(data.error);
      success = "Authentication successful!";
      setTimeout(() => dispatch("switchPage", "home"), 1000);
    }catch(err){
      loginBtn.disabled = false;
      loginBtn.innerText = "Login";
      error = err.message;
    }
  };

  onMount(() => {
    document.getElementById("username").focus();
  });

</script>

<form class="login-form mt-28" on:submit|preventDefault={login}>
  <label for="username">Username:</label>
  <input type="text" id="username" bind:value={fields.username} required>
  <label for="password">Password:</label>
  <input type="password" id="password" bind:value={fields.password} required>
  <div class="flex flex-row space-x-2 mt-5">
    <Button bind:btn={loginBtn} btnType="submit">Login</Button>
    <Button type="secondary" on:click={() => dispatch("switchPage", "register")}>Register</Button>
  </div>
  <SuccessMsg {success}/>
  <ErrorMsg {error}/>
</form>
