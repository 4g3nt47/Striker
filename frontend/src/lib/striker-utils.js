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
