/*
* DataSink
* (C) 1999-2007 Jack Lloyd
*     2017 Philippe Lieser
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DATA_SINK_H_
#define BOTAN_DATA_SINK_H_

#include <botan/filter.h>
#include <memory>
#include <iosfwd>

namespace Botan {

/**
* This class represents abstract data sink objects.
*/
class BOTAN_PUBLIC_API(2,0) DataSink : public Filter
   {
   public:
      bool attachable() override { return false; }
      DataSink() = default;
      virtual ~DataSink() = default;

      DataSink& operator=(const DataSink&) = delete;
      DataSink(const DataSink&) = delete;
   };

/**
* This class represents a data sink which writes its output to a stream.
*/
class BOTAN_PUBLIC_API(2,0) DataSink_Stream final : public DataSink
   {
   public:
      /**
      * Construct a DataSink_Stream from a stream.
      * @param stream the stream to write to
      * @param name identifier
      */
      DataSink_Stream(std::ostream& stream,
                      const std::string& name = "<std::ostream>");

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)

      /**
      * Construct a DataSink_Stream from a filesystem path name.
      * @param pathname the name of the file to open a stream to
      * @param use_binary indicates whether to treat the file
      * as a binary file or not
      */
      DataSink_Stream(const std::string& pathname,
                      bool use_binary = false);
#endif

      std::string name() const override { return m_identifier; }

      void write(const uint8_t[], size_t) override;

      void end_msg() override;

      ~DataSink_Stream();

   private:
      const std::string m_identifier;

      // May be null, if m_sink was an external reference
      std::unique_ptr<std::ostream> m_sink_memory;
      std::ostream& m_sink;
   };

}

#endif
