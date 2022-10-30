<script>

  /**
   * @file Team chat component.
   * @author Umar Abdul
   * Props:
   *        messages - An array of all messages.
   * 
   * Events:
   *        sendMessage - When sending a new message.
   */

  import {onMount, createEventDispatcher} from 'svelte';
  import {formatDate} from '../lib/striker-utils.js';

  export let messages = [];

  const dispatch = createEventDispatcher();
  let msgCount = 0;
  let input = "";

  const updateChatMessages = (messages) => {

    if (messages.length === msgCount)
      return;
    msgCount = messages.length;
    scrollToBottom();
  };

  const scrollToBottom = () => {

    setTimeout(() => {
      let elem = document.getElementById('messages');
      if (elem)
        elem.scrollTop = elem.scrollHeight;
    }, 70);
  };

  const updateChatSize = () => {

    let output = document.getElementById('messages');
    let wrapper = document.getElementById('wrapper');
    if (output && wrapper)
      output.style['height'] = `${wrapper.offsetHeight - 40}px`;
  };

  const inputKeyup = (e) => {

    if (!(e.key === 'Enter' || e.keyCode === 13))
      return;
    if (!input)
      return;
    if (input === "/clear"){
      dispatch("clearMessages");
      msgCount = 0;
    }else{
      dispatch("sendMessage", input);
    }
    input = "";
  };

  $: updateChatMessages(messages);

  onMount(() => {
    document.getElementById('input').focus();
    updateChatSize();
    window.onresize = updateChatSize;
  });

</script>

<div id="wrapper" class="max-h-full min-h-full font-mono text-md">
  <!-- <textarea id="messages" class="no-scrollbar bg-gray-900 text-white" bind:value={chatMessages} readonly></textarea> -->
  <div id="messages" class="border-2 border-green-700 p-1 no-scrollbar overflow-y-auto bg-gray-900 whitespace-pre-wrap break-all text-white">
    {#each messages as msg}
      {#if (msg.username && msg.date)}
        <p><span class="text-yellow-400">{formatDate(msg.date)}</span><span class="text-green-400">{msg.username.padStart(12, " ")}</span><span class="text-yellow-400">{ " > "}</span>{msg.message}</p>
      {:else}
        <p class="text-green-400">{msg.message}</p>
      {/if}
    {/each}
  </div>
  <input id="input" class="bg-gray-300" type="text" bind:value={input} autocomplete="off" on:keyup={inputKeyup} placeholder="Message...">
</div>
