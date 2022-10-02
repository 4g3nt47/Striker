<script>

  import {onMount, createEventDispatcher} from 'svelte';
  import Button from './Button.svelte';
  import SuccessMsg from './SuccessMsg.svelte';
  import ErrorMsg from './ErrorMsg.svelte';

  export let session = {};
  const dispatch = createEventDispatcher();
  const fields = {username: "", password: "", confPassword: "", regKey: ""};
  let registerBtn = null;
  let success = "", error = "";

  const register = async () => {
    
    success = "";
    error = "";
    if (fields.password !== fields.confPassword){
      error = "Passwords must match!";
      return;
    }
    registerBtn.innerText = "Registering...";
    registerBtn.disabled = true;
    try{
      const res = await fetch(`${session.api}/user/register`, {
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
      success = "Account created successfully!";
      setTimeout(() => dispatch("switchPage", "login"), 1000);
    }catch(err){
      registerBtn.disabled = false;
      registerBtn.innerText = "Register";
      error = err.message;
    }
  };

  onMount(() => {
    document.getElementById("username").focus();
  });

</script>

<form class="signup-form mt-28" on:submit|preventDefault={register}>
  <label for="username">Username:</label>
  <input type="text" id="username" bind:value={fields.username} autocomplete="off" placeholder="Username..." required>
  <label for="password">Password:</label>
  <input type="password" id="password" bind:value={fields.password} placeholder="Password..." required>
  <label for="conf-password">Confirm password:</label>
  <input type="password" id="conf-password" bind:value={fields.confPassword} placeholder="Password..." required>
  <label for="reg-key">Registration key:</label>
  <input type="password" id="reg-key" bind:value={fields.regKey} placeholder="Registration key..." required>
  <div class="flex flex-row space-x-2 mt-5">
    <Button bind:btn={registerBtn} btnType="submit">Register</Button>
    <Button type="secondary" on:click={() => dispatch("switchPage", "login")}>Login</Button>
  </div>
  <SuccessMsg {success} textColor="text-white"/>
  <ErrorMsg {error} textColor="text-white"/>
</form>
