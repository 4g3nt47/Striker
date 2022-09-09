<script>
  
  /**
   * @file For displaying error messages.
   * @author Umar Abdul (https://github.com/4g3nt47)
   * Props:
   *        error - The error message. Only displayed when truthy.
   *        textColor - For overriding the text color.
   */

  import {slide} from 'svelte/transition';
  export let error = "";
  export let textColor = "text-black";

</script>

{#if (error)}
  <div transition:slide|local={{duration: 300}} class={`bg-red-500 mt-5 py-1 pl-2 bg-opacity-20 border-l-4 border-red-800 ${textColor}`}>
    {error}
  </div>
{/if}
