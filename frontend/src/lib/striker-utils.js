/**
 * @file Shared utilities for Striker C2 Frontend
 * @author Umar Abdul
 */

/**
 * Format a date to printable string.
 * @param {number} epoch - Date in epoch milliseconds.
 * @return {string} The formatted date. Format: day/month/year hour:min:secs
 */
export const formatDate = (epoch) => {

  let date = new Date(epoch);
  return `${date.getDate().toString().padStart(2, 0)}/${(date.getMonth() + 1).toString().padStart(2, 0)}/${date.getFullYear()} ${date.getHours().toString().padStart(2, 0)}:${date.getMinutes().toString().padStart(2, 0)}:${date.getSeconds().toString().padStart(2, 0)}`;
};

/**
 * Format a time duration into a printable string.
 * @param {number} duration - The duration, in milliseconds
 * @return {string} The formatted duration (e.g: '30 minutes')
 */
export let formatDuration = (duration) => {

  if (duration < 1000)
    return `${duration.toFixed(2)} milliseconds`;
  if (duration < (1000 * 60))
    return `${(duration / (1000)).toFixed(2)} seconds`;
  if (duration < (1000 * 60 * 60))
    return `${(duration / (1000 * 60)).toFixed(2)} minutes`;
  if (duration < (1000 * 60 * 60 * 24))
    return `${(duration / (1000 * 60 * 60)).toFixed(2)} hours`;
  return `${(duration / (1000 * 60 * 60 * 24)).toFixed(2)} days`;
};
