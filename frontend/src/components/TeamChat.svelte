<script>
  
  import {onMount, createEventDispatcher} from 'svelte';

  export let messages = [];

  const dispatch = createEventDispatcher();
  let chatMessages = "";
  let msgCount = 0;
  let input = "";

  const updateChatMessages = (messages) => {

    if (messages.length === msgCount)
      return;
    msgCount = messages.length;
    chatMessages = "";
    for (let i = 0; i < msgCount; i++){
      let message = messages[i];
      if (typeof(message) === 'string'){
        chatMessages += message + (i == (msgCount - 1) ? "" : "\n");
      }else{
        let date = new Date(messages[i].date);
        chatMessages += `${date.getDate().toString().padStart(2, '0')}/${(date.getMonth() + 1).toString().padStart(2, '0')} - ${date.toLocaleTimeString()} ${messages[i].username.padStart(10, " ")}:  ${messages[i].message + (i == (msgCount - 1) ? "" : "\n")}`;
      }
    }
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
      output.rows = wrapper.offsetHeight / 32;
  };

  const inputKeyup = (e) => {

    if (!(e.key === 'Enter' || e.keyCode === 13))
      return;
    if (!input)
      return;
    dispatch("sendMessage", input);
    input = "";
  };

  onMount(() => document.getElementById('input').focus());

  $: updateChatMessages(messages);
  window.onresize = updateChatSize;
  setTimeout(updateChatSize, 70);

</script>

<div id="wrapper" class="h-full font-mono text-md">
  <textarea id="messages" class="no-scrollbar bg-gray-900 text-white" bind:value={chatMessages} readonly></textarea>
  <input id="input" class="bg-gray-300" type="text" bind:value={input} autocomplete="off" on:keyup={inputKeyup} placeholder="Message...">
</div>
