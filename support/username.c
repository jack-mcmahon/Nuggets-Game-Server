/*
 * username - a module that provides helper functions to make usernames.
 *
 * See username.h for detailed interface description for each function.
 *
 * Compile with -DUNIT_TEST for a standalone unit test; see below.
 *
 * Marvin Escobar Barajas, March 2022
 */

#include "username.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************** global functions ****************/
/* that is, visible outside this file */
/* see username.h for comments about exported functions */

/**************** normalizeUserName() ****************/
char* normalizeUserName(const char* username, int maxNameLength)
{
    if (username == NULL || maxNameLength < 1) {  // defensive
        fprintf(stderr,
                "usage: username is NULL or maxNameLength is less than 1.\n");
        return NULL;
    }

    char* newUserName = malloc(sizeof(char) * strlen(username) + 1);

    if (newUserName == NULL) {  // defensive
        fprintf(stderr, "System out of memory.\n");
        return NULL;
    }

    strcpy(newUserName, username);

    if (strlen(username) > maxNameLength) {
        // truncate string
        newUserName[maxNameLength] = '\0';
    }

    // loop through and replace non-graphical characters with '_'
    for (char* c = newUserName; *c != '\0'; c++) {
        if (!isblank(*c) && !isgraph(*c)) {
            *c = '_';
        }
    }

    return newUserName;
}

bool isEmpty(const char* str)
{
    if (str == NULL) {  // defensive
        return false;
    }

    for (const char* c = str; *c != '\0'; c++) {
        if (!isblank(*c)) {
            return false;
        }
    }

    return true;
}

/* ****************************************************************** */
/* ************************* UNIT_TEST ****************************** */
/*
 * This unit test tests out the helper functions used to
 * build valid Player usernames.
 *
 * Run with valgrind:
 * myvalgrind ./usernametest
 */

#ifdef UNIT_TEST

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    // testing isEmpty function
    printf("Testing isEmpty()\n");
    assert(isEmpty("                "));
    assert(!isEmpty("Hello world"));
    printf("Tests passed successfully.\n");

    // testing normalizeUserName
    printf("Testing normalizeUserName()\n");
    char* name = normalizeUserName("Marvin Escobar Barajas", 5);
    printf("Normalized %s to %s.\n", "Marvin Escobar Barajas", name);
    free(name);
    printf("Attempting to normalize username to 0 characters.\n");
    name = normalizeUserName("Marvin Escobar Barajas", 0);
    printf("Tests passed successfully.\n");

    return 0;
}

#endif  // UNIT_TEST