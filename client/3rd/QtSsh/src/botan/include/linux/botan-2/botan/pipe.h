/*
* Pipe
* (C) 1999-2007 Jack Lloyd
*     2012 Markus Wanner
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PIPE_H_
#define BOTAN_PIPE_H_

#include <botan/data_src.h>
#include <botan/exceptn.h>
#include <initializer_list>
#include <iosfwd>

namespace Botan {

class Filter;
class Output_Buffers;

/**
* This class represents pipe objects.
* A set of filters can be placed into a pipe, and information flows
* through the pipe until it reaches the end, where the output is
* collected for retrieval.  If you're familiar with the Unix shell
* environment, this design will sound quite familiar.
*/
class BOTAN_PUBLIC_API(2,0) Pipe final : public DataSource
   {
   public:
      /**
      * An opaque type that identifies a message in this Pipe
      */
      typedef size_t message_id;

      /**
      * Exception if you use an invalid message as an argument to
      * read, remaining, etc
      */
      class BOTAN_PUBLIC_API(2,0) Invalid_Message_Number final : public Invalid_Argument
         {
         public:
            /**
            * @param where the error occurred
            * @param msg the invalid message id that was used
            */
            Invalid_Message_Number(const std::string& where, message_id msg) :
               Invalid_Argument("Pipe::" + where + ": Invalid message number " +
                                std::to_string(msg))
               {}
         };

      /**
      * A meta-id for whatever the last message is
      */
      static const message_id LAST_MESSAGE;

      /**
      * A meta-id for the default message (set with set_default_msg)
      */
      static const message_id DEFAULT_MESSAGE;

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the byte array to write
      * @param length the length of the byte array in
      */
      void write(const uint8_t in[], size_t length);

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the secure_vector containing the data to write
      */
      void write(const secure_vector<uint8_t>& in)
         { write(in.data(), in.size()); }

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the std::vector containing the data to write
      */
      void write(const std::vector<uint8_t>& in)
         { write(in.data(), in.size()); }

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the string containing the data to write
      */
      void write(const std::string& in);

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in the DataSource to read the data from
      */
      void write(DataSource& in);

      /**
      * Write input to the pipe, i.e. to its first filter.
      * @param in a single byte to be written
      */
      void write(uint8_t in);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the byte array containing the data to write
      * @param length the length of the byte array to write
      */
      void process_msg(const uint8_t in[], size_t length);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the secure_vector containing the data to write
      */
      void process_msg(const secure_vector<uint8_t>& in);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the secure_vector containing the data to write
      */
      void process_msg(const std::vector<uint8_t>& in);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the string containing the data to write
      */
      void process_msg(const std::string& in);

      /**
      * Perform start_msg(), write() and end_msg() sequentially.
      * @param in the DataSource providing the data to write
      */
      void process_msg(DataSource& in);

      /**
      * Find out how many bytes are ready to read.
      * @param msg the number identifying the message
      * for which the information is desired
      * @return number of bytes that can still be read
      */
      size_t remaining(message_id msg = DEFAULT_MESSAGE) const BOTAN_WARN_UNUSED_RESULT;

      /**
      * Read the default message from the pipe. Moves the internal
      * offset so that every call to read will return a new portion of
      * the message.
      *
      * @param output the byte array to write the read bytes to
      * @param length the length of the byte array output
      * @return number of bytes actually read into output
      */
      size_t read(uint8_t output[], size_t length) override BOTAN_WARN_UNUSED_RESULT;

      /**
      * Read a specified message from the pipe. Moves the internal
      * offset so that every call to read will return a new portion of
      * the message.
      * @param output the byte array to write the read bytes to
      * @param length the length of the byte array output
      * @param msg the number identifying the message to read from
      * @return number of bytes actually read into output
      */
      size_t read(uint8_t output[], size_t length, message_id msg) BOTAN_WARN_UNUSED_RESULT;

      /**
      * Read a single byte from the pipe. Moves the internal offset so
      * that every call to read will return a new portion of the
      * message.
      *
      * @param output the byte to write the result to
      * @param msg the message to read from
      * @return number of bytes actually read into output
      */
      size_t read(uint8_t& output, message_id msg = DEFAULT_MESSAGE) BOTAN_WARN_UNUSED_RESULT;

      /**
      * Read the full contents of the pipe.
      * @param msg the number identifying the message to read from
      * @return secure_vector holding the contents of the pipe
      */
      secure_vector<uint8_t> read_all(message_id msg = DEFAULT_MESSAGE) BOTAN_WARN_UNUSED_RESULT;

      /**
      * Read the full contents of the pipe.
      * @param msg the number identifying the message to read from
      * @return string holding the contents of the pipe
      */
      std::string read_all_as_string(message_id msg = DEFAULT_MESSAGE) BOTAN_WARN_UNUSED_RESULT;

      /**
      * Read from the default message but do not modify the internal
      * offset. Consecutive calls to peek() will return portions of
      * the message starting at the same position.
      * @param output the byte array to write the peeked message part to
      * @param length the length of the byte array output
      * @param offset the offset from the current position in message
      * @return number of bytes actually peeked and written into output
      */
      size_t peek(uint8_t output[], size_t length, size_t offset) const override BOTAN_WARN_UNUSED_RESULT;

      /** Read from the specified message but do not modify the
      * internal offset. Consecutive calls to peek() will return
      * portions of the message starting at the same position.
      * @param output the byte array to write the peeked message part to
      * @param length the length of the byte array output
      * @param offset the offset from the current position in message
      * @param msg the number identifying the message to peek from
      * @return number of bytes actually peeked and written into output
      */
      size_t peek(uint8_t output[], size_t length,
                  size_t offset, message_id msg) const BOTAN_WARN_UNUSED_RESULT;

      /** Read a single byte from the specified message but do not
      * modify the internal offset. Consecutive calls to peek() will
      * return portions of the message starting at the same position.
      * @param output the byte to write the peeked message byte to
      * @param offset the offset from the current position in message
      * @param msg the number identifying the message to peek from
      * @return number of bytes actually peeked and written into output
      */
      size_t peek(uint8_t& output, size_t offset,
                  message_id msg = DEFAULT_MESSAGE) const BOTAN_WARN_UNUSED_RESULT;

      /**
      * @return the number of bytes read from the default message.
      */
      size_t get_bytes_read() const override;

      /**
      * @return the number of bytes read from the specified message.
      */
      size_t get_bytes_read(message_id msg) const;

      bool check_available(size_t n) override;
      bool check_available_msg(size_t n, message_id msg);

      /**
      * @return currently set default message
      */
      size_t default_msg() const { return m_default_read; }

      /**
      * Set the default message
      * @param msg the number identifying the message which is going to
      * be the new default message
      */
      void set_default_msg(message_id msg);

      /**
      * Get the number of messages the are in this pipe.
      * @return number of messages the are in this pipe
      */
      message_id message_count() const;

      /**
      * Test whether this pipe has any data that can be read from.
      * @return true if there is more data to read, false otherwise
      */
      bool end_of_data() const override;

      /**
      * Start a new message in the pipe. A potential other message in this pipe
      * must be closed with end_msg() before this function may be called.
      */
      void start_msg();

      /**
      * End the current message.
      */
      void end_msg();

      /**
      * Insert a new filter at the front of the pipe
      * Deprecated because runtime modification of Pipes is deprecated.
      * You can instead use prepend_filter which only works before the first
      * message is processed.
      * @param filt the new filter to insert
      */
      BOTAN_DEPRECATED("Runtime modification of Pipe deprecated")
      void prepend(Filter* filt);

      /**
      * Insert a new filter at the back of the pipe
      * Deprecated because runtime modification of Pipes is deprecated.
      * You can instead use append_filter which only works before the first
      * message is processed.
      * @param filt the new filter to insert
      */
      BOTAN_DEPRECATED("Runtime modification of Pipe deprecated")
      void append(Filter* filt);

      /**
      * Remove the first filter at the front of the pipe.
      */
      BOTAN_DEPRECATED("Runtime modification of Pipe deprecated")
      void pop();

      /**
      * Reset this pipe to an empty pipe.
      */
      BOTAN_DEPRECATED("Runtime modification of Pipe deprecated")
      void reset();

      /**
      * Append a new filter onto the filter sequence. This may only be
      * called immediately after initial construction, before _any_
      * calls to start_msg have been made.
      *
      * This function (unlike append) is not deprecated, as it allows
      * only modification of the pipe at initialization (before use)
      * rather than after messages have been processed.
      */
      void append_filter(Filter* filt);

      /**
      * Prepend a new filter onto the filter sequence. This may only be
      * called immediately after initial construction, before _any_
      * calls to start_msg have been made.
      *
      * This function (unlike prepend) is not deprecated, as it allows
      * only modification of the pipe at initialization (before use)
      * rather than after messages have been processed.
      */
      void prepend_filter(Filter* filt);

      /**
      * Construct a Pipe of up to four filters. The filters are set up
      * in the same order as the arguments.
      */
      Pipe(Filter* = nullptr, Filter* = nullptr,
           Filter* = nullptr, Filter* = nullptr);

      /**
      * Construct a Pipe from a list of filters
      * @param filters the set of filters to use
      */
      explicit Pipe(std::initializer_list<Filter*> filters);

      Pipe(const Pipe&) = delete;
      Pipe& operator=(const Pipe&) = delete;

      ~Pipe();
   private:
      void destruct(Filter*);
      void do_append(Filter* filt);
      void do_prepend(Filter* filt);
      void find_endpoints(Filter*);
      void clear_endpoints(Filter*);

      message_id get_message_no(const std::string&, message_id) const;

      Filter* m_pipe;
      std::unique_ptr<Output_Buffers> m_outputs;
      message_id m_default_read;
      bool m_inside_msg;
   };

/**
* Stream output operator; dumps the results from pipe's default
* message to the output stream.
* @param out an output stream
* @param pipe the pipe
*/
BOTAN_PUBLIC_API(2,0) std::ostream& operator<<(std::ostream& out, Pipe& pipe);

/**
* Stream input operator; dumps the remaining bytes of input
* to the (assumed open) pipe message.
* @param in the input stream
* @param pipe the pipe
*/
BOTAN_PUBLIC_API(2,0) std::istream& operator>>(std::istream& in, Pipe& pipe);

}

#if defined(BOTAN_HAS_PIPE_UNIXFD_IO)
  #include <botan/fd_unix.h>
#endif

#endif
