/*
* Output Buffer
* (C) 1999-2007 Jack Lloyd
*     2012 Markus Wanner
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_OUTPUT_BUFFER_H_
#define BOTAN_OUTPUT_BUFFER_H_

#include <botan/types.h>
#include <botan/pipe.h>
#include <deque>

namespace Botan {

/**
* Container of output buffers for Pipe
*/
class Output_Buffers final
   {
   public:
      size_t read(uint8_t[], size_t, Pipe::message_id);
      size_t peek(uint8_t[], size_t, size_t, Pipe::message_id) const;
      size_t get_bytes_read(Pipe::message_id) const;
      size_t remaining(Pipe::message_id) const;

      void add(class SecureQueue*);
      void retire();

      Pipe::message_id message_count() const;

      Output_Buffers();
   private:
      class SecureQueue* get(Pipe::message_id) const;

      std::deque<std::unique_ptr<SecureQueue>> m_buffers;
      Pipe::message_id m_offset;
   };

}

#endif
