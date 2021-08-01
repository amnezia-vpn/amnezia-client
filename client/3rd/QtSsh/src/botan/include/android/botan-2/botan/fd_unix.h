/*
* Pipe I/O for Unix
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PIPE_UNIXFD_H_
#define BOTAN_PIPE_UNIXFD_H_

#include <botan/types.h>

namespace Botan {

class Pipe;

/**
* Stream output operator; dumps the results from pipe's default
* message to the output stream.
* @param out file descriptor for an open output stream
* @param pipe the pipe
*/
int BOTAN_PUBLIC_API(2,0) operator<<(int out, Pipe& pipe);

/**
* File descriptor input operator; dumps the remaining bytes of input
* to the (assumed open) pipe message.
* @param in file descriptor for an open input stream
* @param pipe the pipe
*/
int BOTAN_PUBLIC_API(2,0) operator>>(int in, Pipe& pipe);

}

#endif
