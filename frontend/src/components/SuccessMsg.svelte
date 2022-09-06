<script>

  /**
   * @file For displaying success messages.
   * @author Umar Abdul (https://github.com/4g3nt47)
   * Props:
   *        success - The success message. Only displayed when truthy.
   */

  import {slide} from 'svelte/transition';
  export let success = "";

</script>

{#if (success)}
  <div transition:slide|local={{duration: 300}} class="bg-green-500 mt-5 py-1 pl-2 bg-opacity-20 text-black border-l-4 border-green-800">
    {success}
  </div>
{/if}
