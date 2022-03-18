/* 
 * username - a module that provides helper functions to make usernames. 
 * 
 * Provides functions to normalize usernames and determine whether a
 * given username is only whitespace.
 *
 * David Kotz - May 2019
 */

#ifndef _USERNAME_H_
#define _USERNAME_H_

#include <stdbool.h>

/******************************************/
/* normalizeUserName:  truncates an over-length username to maxNameLength 
 *                     characters and replaces underscore for any character where 
 *                     isgraph() and isblank() are false
 * 
 * Caller provides:
 *   non-null string username and a valid maxNameLength int (must be at least one char)
 * Function returns:
 *   malloc'd string that represents a normalized username
 *   NULL if error
 * Caller expectations:
 *   call free() on the string username that is produced.
 */
char* normalizeUserName(const char* username, int maxNameLength);

/******************************************/
/* isEmpty: A helper function that returns whether a given string is only whitespace.
 * Caller provides:
 *   non-NULL string
 * Function returns:
 *   false if str is NULL or contains character that is not whitespace.
 *   true if string is completely whitespace
 * . 
 */
bool isEmpty(const char* str);


#endif // _USERNAME_H_