#include <string.h>
#include <ctype.h>

/** \brief Convert string to uppercase
 *
 * Converts the given \0-terminated string to upper case.
 *
 * \param s Pointer to \0-terminated C string
 *
 * \return Pointer to the string converted to upper case
 *
 */
char* strtoupper(char* s) {
  char* p = s;
  while (*p != '\0') {
    *p = toupper(*p);
    p++;
  }

  return s;
}


/** \brief Convert string to upper case
 *
 * Converts at most n characters of the given \0-terminated string to
 * upper case.
 *
 * \param s Pointer to \0-terminated C string
 * \param n Max numer of characters to convert
 *
 * \return Pointer to the string converted to upper case
 *
 */
char* strntoupper(char* s, size_t n) {
  if (n == 0)
    return s;

  char *p = s;
  while (n > 0 && *p != '\0') {
    *p = toupper(*p);
    p++;
    n--;
  }

  return s;
}

/** \brief Convert string to lower case
 *
 * Converts the given \0-terminated string to lower case.
 *
 * \param s Pointer to \0-terminated C string
 *
 * \return Pointer to the string converted to lower case
 *
 */
char* strtolower(char* s) {
  char* p = s;
  while (*p != '\0') {
    *p = tolower(*p);
    p++;
  }

  return s;
}

/** \brief Convert string to lower case
 *
 * Converts at most n characters of the given \0-terminated string to
 * lower case.
 *
 * \param s Pointer to \0-terminated C string
 * \param n Max numer of characters to convert
 *
 * \return Pointer to the string converted to lower case
 *
 */
char* strntolower(char* s, size_t n) {
  if (n == 0)
    return s;

  char *p = s;
  while (n > 0 && *p != '\0') {
    *p = tolower(*p);
    p++;
    n--;
  }

  return s;
}
