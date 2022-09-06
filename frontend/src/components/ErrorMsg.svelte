<script>
  
  /**
   * @file For displaying error messages.
   * @author Umar Abdul (https://github.com/4g3nt47)
   * Props:
   *        error - The error message. Only displayed when truthy.
   */

  import {slide} from 'svelte/transition';
  export let error = "";

</script>

{#if (error)}
  <div transition:slide|local={{duration: 300}} class="bg-red-500 mt-5 py-1 pl-2 bg-opacity-20 text-black border-l-4 border-red-800">
    {error}
  </div>
{/if}
