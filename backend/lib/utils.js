/**
 * @file Some utility functions
 * @author Umar Abdul
 */

/**
 * Ouput a given message to the console.
 * @param {string} msg - The message to output
 */
export const output = (msg) => {
  console.log(`[${new Date().toLocaleTimeString()}]  ${msg}`);
};


/**
 * Split a string at whitespace. Does not split escaped whitespaces.
 * "some\ long string" => ["some long", "string"]
 * @param {string} str - The string to split
 * @return {object} An array of split values.
 */
export const spaceSplit = (str) => {

  let len = str.length;
  let chunks = [];
  let chunk = "";
  for (let i = 0; str[i] !== undefined; i++){
    if (i === 0){
      chunk += str[i];
      continue;
    }
    if (str[i] === ' ' && str[i - 1] !== '\\'){
      chunks.push(chunk.replaceAll("\\ ", " "));
      chunk = "";
      continue;
    }
    chunk += str[i];
  }
  if (chunk.length !== 0)
    chunks.push(chunk.replaceAll("\\ ", " "));
  return chunks;
};
