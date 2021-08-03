/*
* PKCS#11 Module/Slot/Session
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_TYPES_H_
#define BOTAN_P11_TYPES_H_

#include <botan/p11.h>
#include <string>
#include <memory>
#include <functional>
#include <utility>

namespace Botan {

class Dynamically_Loaded_Library;

namespace PKCS11 {

/**
* Loads the PKCS#11 shared library
* Calls C_Initialize on load and C_Finalize on destruction
*/
class BOTAN_PUBLIC_API(2,0) Module final
   {
   public:
      /**
      * Loads the shared library and calls C_Initialize
      * @param file_path the path to the PKCS#11 shared library
      * @param init_args flags to use for `C_Initialize`
      */
      Module(const std::string& file_path, C_InitializeArgs init_args = { nullptr, nullptr, nullptr, nullptr, static_cast< CK_FLAGS >(Flag::OsLockingOk), nullptr });

      Module(Module&& other);
      Module& operator=(Module&& other) = delete;

      // Dtor calls C_Finalize(). A copy could be deleted while the origin still exists
      // Furthermore std::unique_ptr member -> not copyable
      Module(const Module& other) = delete;
      Module& operator=(const Module& other) = delete;

      /// Calls C_Finalize()
      ~Module() noexcept;

      /**
      * Reloads the module and reinitializes it
      * @param init_args flags to use for `C_Initialize`
      */
      void reload(C_InitializeArgs init_args = { nullptr, nullptr, nullptr, nullptr, static_cast< CK_FLAGS >(Flag::OsLockingOk), nullptr });

      inline LowLevel* operator->() const
         {
         return m_low_level.get();
         }

      /// @return general information about Cryptoki
      inline Info get_info() const
         {
         Info info;
         m_low_level->C_GetInfo(&info);
         return info;
         }

   private:
      const std::string m_file_path;
      FunctionListPtr m_func_list = nullptr;
      std::unique_ptr<Dynamically_Loaded_Library> m_library;
      std::unique_ptr<LowLevel> m_low_level = nullptr;
   };

/// Represents a PKCS#11 Slot, i.e., a card reader
class BOTAN_PUBLIC_API(2,0) Slot final
   {
   public:
      /**
      * @param module the PKCS#11 module to use
      * @param slot_id the slot id to use
      */
      Slot(Module& module, SlotId slot_id);

      /// @return a reference to the module that is used
      inline Module& module() const
         {
         return m_module;
         }

      /// @return the slot id
      inline SlotId slot_id() const
         {
         return m_slot_id;
         }

      /**
      * Get available slots
      * @param module the module to use
      * @param token_present true if only slots with attached tokens should be returned, false for all slots
      * @return a list of available slots (calls C_GetSlotList)
      */
      static std::vector<SlotId> get_available_slots(Module& module, bool token_present);

      /// @return information about the slot (`C_GetSlotInfo`)
      SlotInfo get_slot_info() const;

      /// Obtains a list of mechanism types supported by the slot (`C_GetMechanismList`)
      std::vector<MechanismType> get_mechanism_list() const;

      /// Obtains information about a particular mechanism possibly supported by a slot (`C_GetMechanismInfo`)
      MechanismInfo get_mechanism_info(MechanismType mechanism_type) const;

      /// Obtains information about a particular token in the system (`C_GetTokenInfo`)
      TokenInfo get_token_info() const;

      /**
      * Calls `C_InitToken` to initialize the token
      * @param label the label for the token (must not exceed 32 bytes according to PKCS#11)
      * @param so_pin the PIN of the security officer
      */
      void initialize(const std::string& label, const secure_string& so_pin) const;

   private:
      const std::reference_wrapper<Module> m_module;
      const SlotId m_slot_id;
   };

/// Represents a PKCS#11 session
class BOTAN_PUBLIC_API(2,0) Session final
   {
   public:
      /**
      * @param slot the slot to use
      * @param read_only true if the session should be read only, false to create a read-write session
      */
      Session(Slot& slot, bool read_only);

      /**
      * @param slot the slot to use
      * @param flags the flags to use for the session. Remark: Flag::SerialSession is mandatory
      * @param callback_data application-defined pointer to be passed to the notification callback
      * @param notify_callback address of the notification callback function
      */
      Session(Slot& slot, Flags flags, VoidPtr callback_data, Notify notify_callback);

      /// Takes ownership of a session
      Session(Slot& slot, SessionHandle handle);

      Session(Session&& other) = default;
      Session& operator=(Session&& other) = delete;

      // Dtor calls C_CloseSession() and eventually C_Logout. A copy could close the session while the origin still exists
      Session(const Session& other) = delete;
      Session& operator=(const Session& other) = delete;

      /// Logout user and close the session on destruction
      ~Session() noexcept;

      /// @return a reference to the slot
      inline const Slot& slot() const
         {
         return m_slot;
         }

      /// @return the session handle of this session
      inline SessionHandle handle() const
         {
         return m_handle;
         }

      /// @return a reference to the used module
      inline Module& module() const
         {
         return m_slot.module();
         }

      /// @return the released session handle
      SessionHandle release();

      /**
      * Login to this session
      * @param userType the user type to use for the login
      * @param pin the PIN of the user
      */
      void login(UserType userType, const secure_string& pin);

      /// Logout from this session
      void logoff();

      /// @return information about this session
      SessionInfo get_info() const;

      /// Calls `C_SetPIN` to change the PIN using the old PIN (requires a logged in session)
      void set_pin(const secure_string& old_pin, const secure_string& new_pin) const;

      /// Calls `C_InitPIN` to change or initialize the PIN using the SO_PIN (requires a logged in session)
      void init_pin(const secure_string& new_pin);

   private:
      const Slot& m_slot;
      SessionHandle m_handle;
      bool m_logged_in;
   };

}
}

#endif
