#pragma once
/** Oh wow I already have a lookup.h in this project what do you know */
#ifndef TOMO_LOOKUP_H
#define TOMO_LOOKUP_H

#include <stdexcept>

/** TODO: Only allow connections to the same subnet? Ask for confirmation if an
 *      attempt is made to connect to the internet?
 */


/** The lookup mechanism must take the patient's name in DICOM format, and
 *  return either:
 *    - "NOT FOUND" if the patient's name is not found or cannot be mapped
 *    - A list of MRN's mapped to this name, each separated by a single newline
 *      char. There is no currently no facility for disambiguating these IDs
 * 
 *  Because name clashes cannot currently be resolved, the query format may
 *  change in the future to accomodate patient DOB as well.
 */


namespace tomo {


/** @brief Look up @p name in whatever external database is accessible to this
 *      function
 *  @param host
 *      Hostname of the MRN server
 *  @param port
 *      Port to connect to
 *  @param name
 *      On entry, a pointer to a buffer containing the patient name string in
 *      DICOM format. On output, this will contain the result
 *  @param len
 *      The size of @p name in bytes
 *  @throws std::runtime_error if connection cannot be established, or if the
 *      name was not found
 */
void lookup_name(const char *host, uint16_t port, char *name, size_t len);


};


#endif /* TOMO_LOOKUP_H */
