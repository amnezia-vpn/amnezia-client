/*
* (C) 2017 Fabian Weissberg, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PADDING_H_
#define BOTAN_PADDING_H_

#include <botan/build.h>
#include <string>
#include <vector>

namespace Botan {

/**
* Returns the allowed padding schemes when using the given
* algorithm (key type) for creating digital signatures.
*
* @param algo the algorithm for which to look up supported padding schemes
* @return a vector of supported padding schemes
*/
BOTAN_TEST_API const std::vector<std::string> get_sig_paddings(const std::string algo);

/**
* Returns true iff the given padding scheme is valid for the given
* signature algorithm (key type).
*
* @param algo the signature algorithm to be used
* @param padding the padding scheme to be used
*/
bool sig_algo_and_pad_ok(const std::string algo, const std::string padding);

}

#endif
