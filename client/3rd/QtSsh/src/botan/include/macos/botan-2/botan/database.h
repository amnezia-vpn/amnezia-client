/*
* SQL database interface
* (C) 2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SQL_DATABASE_H_
#define BOTAN_SQL_DATABASE_H_

#include <botan/types.h>
#include <botan/exceptn.h>
#include <string>
#include <chrono>
#include <vector>

namespace Botan {

class BOTAN_PUBLIC_API(2,0) SQL_Database
   {
   public:

      class BOTAN_PUBLIC_API(2,0) SQL_DB_Error final : public Exception
         {
         public:
            explicit SQL_DB_Error(const std::string& what) :
               Exception("SQL database", what),
               m_rc(0)
               {}

            SQL_DB_Error(const std::string& what, int rc) :
               Exception("SQL database", what),
               m_rc(rc)
               {}

            ErrorType error_type() const noexcept override { return Botan::ErrorType::DatabaseError; }

            int error_code() const noexcept override { return m_rc; }
         private:
            int m_rc;
         };

      class BOTAN_PUBLIC_API(2,0) Statement
         {
         public:
            /* Bind statement parameters */
            virtual void bind(int column, const std::string& str) = 0;

            virtual void bind(int column, size_t i) = 0;

            virtual void bind(int column, std::chrono::system_clock::time_point time) = 0;

            virtual void bind(int column, const std::vector<uint8_t>& blob) = 0;

            virtual void bind(int column, const uint8_t* data, size_t len) = 0;

            /* Get output */
            virtual std::pair<const uint8_t*, size_t> get_blob(int column) = 0;

            virtual std::string get_str(int column) = 0;

            virtual size_t get_size_t(int column) = 0;

            /* Run to completion */
            virtual size_t spin() = 0;

            /* Maybe update */
            virtual bool step() = 0;

            virtual ~Statement() = default;
         };

      /*
      * Create a new statement for execution.
      * Use ?1, ?2, ?3, etc for parameters to set later with bind
      */
      virtual std::shared_ptr<Statement> new_statement(const std::string& base_sql) const = 0;

      virtual size_t row_count(const std::string& table_name) = 0;

      virtual void create_table(const std::string& table_schema) = 0;

      virtual ~SQL_Database() = default;
};

}

#endif
