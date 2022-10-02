<script>

  /**
   * @file Reusable button component. 
   * @author Umar Abdul (https://github.com/4g3nt47)
   * Props:
   *        type - The type of the button, either 'primary', 'secondary', 'danger', or 'custom'.
   *        btnType - The value to pass to the 'type' attribute of the button element.
   *        btn - Will be bound to the HTML button element. Used to control the button externally.
   *        custom - Custom classes to use. Only used when `type` is set to `custom`. Should define
   *                 the border color, background color, and text color of the button.
   * Emits:
   *        click - When the button is clicked.
   */

  export let type = "primary";
  export let btnType = "button";
  export let custom = "";
  export let btn = null;

</script>

{#if (type === "primary")}
  <button class="w-full py-1 bg-green-900 border-2 border-green-900 bg-opacity-75 hover:bg-opacity-90 duration-200 text-white text-lg rounded-md" type={btnType} bind:this={btn} on:click>
    <slot></slot>
  </button>
{:else if (type === "secondary")}
  <button class="w-full py-1 bg-gray-700 border-2 border-gray-700 bg-opacity-75 hover:bg-opacity-90 duration-200 text-white text-lg rounded-md" type={btnType} bind:this={btn} on:click>
    <slot></slot>
  </button>
{:else if (type === "danger")}
  <button class="w-full py-1 bg-red-900 border-2 border-red-900 bg-opacity-80 hover:bg-opacity-90 duration-200 text-white text-lg rounded-md" type={btnType} bind:this={btn} on:click>
    <slot></slot>
  </button>
{:else if (type === "custom")}
  <button class={custom + " w-full py-1 border-2 bg-opacity-80 hover:bg-opacity-90 duration-200 text-lg rounded-md"} type={btnType} bind:this={btn} on:click>
    <slot></slot>
  </button>

{/if}
