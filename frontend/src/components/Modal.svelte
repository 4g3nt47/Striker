<script>
  
  /**
   * @file A component for creating a modal. The modal content is received as a slot.
   * This displayed modal can be hidden by simply clicking on the backdrop (the dark, transparent background)
   * @author Umar Abdul
   * Props:
   *        show - A boolean. Decides used to display/hide the modal. Default: false
   *        width - The width class to add pass to the modal. Default: w-2/3
   * Events:
   *        modalClosed - This event is emitted when user closes the modal. The parent is expected to update the `show` prop to false to hide the modal.
   */
  
  import {createEventDispatcher} from 'svelte';

  export let show = false;
  export let width = "w-2/3";
  const dispatch = createEventDispatcher();

  // Called when user clicks on backdrop.
  const hideModal = () => {
    dispatch("modalClosed");
  };

</script>

{#if (show)}
  <div class="absolute left-0 top-0 z-10 w-full h-full bg-black bg-opacity-70" on:click|self={hideModal}>
    <div class={width + " max-h-96 mx-auto mt-32 bg-gray-300 p-3 shadow-lg shadow-black rounded no-scrollbar overflow-y-auto"}>
      <slot></slot>
    </div>
  </div>
{/if}
