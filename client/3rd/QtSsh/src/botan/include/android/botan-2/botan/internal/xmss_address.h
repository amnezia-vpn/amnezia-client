/*
 * XMSS Address
 * (C) 2016 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_ADDRESS_H_
#define BOTAN_XMSS_ADDRESS_H_

#include <botan/types.h>

namespace Botan {

/**
 * Generic XMSS Address type holding 256 Bits of data. Properties
 * of all three address formats L-Tree-Address, Hash-Tree-Address,
 * OTS-Hash-Address can be called depending on the type currently
 * assigned to the XMSS address using set_type().
 **/
class XMSS_Address final
   {
   public:
      /**
       * Distinct types an XMSS_Address can represent. The available types
       * are specified in [1] - 2.5 Hash Function Address Scheme.
       **/
      enum class Type : uint8_t
         {
         None = 255,
         OTS_Hash_Address = 0,
         LTree_Address = 1,
         Hash_Tree_Address = 2
         };

      /**
       * The available modes for an XMSS Address:
       *   - Key_Mode: Used to generate the key.
       *   - Mask_Mode: Sets the n-byte bitmask (OTS-Hash-Address)
       *   - Mask_MSB_Mode: Used to generate the b most significant bytes of
       *     the 2n-byte bitmask (LTree Address and Hash Tree Address).
       *   - Mask_LSB_Mode: Used to generated the b least significant bytes
       *     of the 2n-byte bitmask. (LTree Address and Hash Tree Address).
       **/
      enum class Key_Mask : uint8_t
         {
         Key_Mode = 0,
         Mask_Mode = 1,
         Mask_MSB_Mode = 1,
         Mask_LSB_Mode = 2
         };

      /**
       * Layer Address for XMSS is constantly zero and can not be changed this
       * property is only of relevance to XMSS_MT.
       *
       * @return Layer address, which is constant 0 for XMSS.
       **/
      uint8_t get_layer_addr() const { return 0; }

      /**
       * Layer Address for XMSS is constantly zero and can not be changed this
       * property is only of relevance to XMSS_MT. Calling this method for
       * XMSS will result in an error.
       **/
      void set_layer_addr()
         {
         BOTAN_ASSERT(false, "Only available in XMSS_MT.");
         }

      /**
       * Tree Address for XMSS is constantly zero and can not be changed this
       * property is only of relevance to XMSS_MT.
       *
       * @return Tree address, which is constant 0 for XMSS.
       **/
      uint64_t get_tree_addr() const { return 0; }

      /**
       * Tree Address for XMSS is constantly zero and can not be changed this
       * property is only of relevance to XMSS_MT. Calling this method for
       * XMSS will result in an error.
       **/
      void set_tree_addr()
         {
         BOTAN_ASSERT(false, "Only available in XMSS_MT.");
         }

      /**
       * retrieves the logical type currently assigned to the XMSS Address
       * instance.
       *
       * @return Type of the address (OTS_Hash_Address, LTree_Address or
       *         Hash_Tree_Address)
       **/
      Type get_type() const
         {
         return static_cast<Type>(m_data[15]);
         }

      /**
       * Changes the logical type currently assigned to the XMSS Address
       * instance. Please note that changing the type will automatically
       * reset the 128 LSBs of the Address to zero. This affects the
       * key_mask_mode property as well as all properties identified by
       * XMSS_Address::Property.
       *
       * @param type Type that shall be assigned to the address
       *        (OTS_Hash_Address, LTree_Address or Hash_Tree_Address)
       **/
      void set_type(Type type)
         {
         m_data[15] = static_cast<uint8_t>(type);
         std::fill(m_data.begin() + 16, m_data.end(), static_cast<uint8_t>(0));
         }

      /**
       * Retrieves the mode the address os currently set to. (See
       * XMSS_Address::Key_Mask for details.)
       *
       * @return currently active mode
       **/
      Key_Mask get_key_mask_mode() const
         {
         return Key_Mask(m_data[31]);
         }

      /**
       * Changes the mode the address currently used address mode.
       * (XMSS_Address::Key_Mask for details.)
       *
       * @param value Target mode.
       **/
      void set_key_mask_mode(Key_Mask value)
         {
         BOTAN_ASSERT(value != Key_Mask::Mask_LSB_Mode ||
                      get_type() != Type::OTS_Hash_Address,
                      "Invalid Key_Mask for current XMSS_Address::Type.");
         m_data[31] = static_cast<uint8_t>(value);
         }

      /**
       * Retrieve the index of the OTS key pair within the tree. A call to
       * this method is only valid, if the address type is set to
       * Type::OTS_Hash_Address.
       *
       * @return index of OTS key pair.
       **/
      uint32_t get_ots_address() const
         {
         BOTAN_ASSERT(get_type() == Type::OTS_Hash_Address,
                      "get_ots_address() requires XMSS_Address::Type::"
                      "OTS_Hash_Address.");
         return get_hi32(2);
         }

      /**
       * Sets the index of the OTS key pair within the tree. A call to this
       * method is only valid, if the address type is set to
       * Type::OTS_Hash_Address.
       *
       * @param value index of OTS key pair.
       **/
      void set_ots_address(uint32_t value)
         {
         BOTAN_ASSERT(get_type() == Type::OTS_Hash_Address,
                      "set_ots_address() requires XMSS_Address::Type::"
                      "OTS_Hash_Address.");
         set_hi32(2, value);
         }

      /**
       * Retrieves the index of the leaf computed with this LTree. A call to
       * this method is only valid, if the address type is set to
       * Type::LTree_Address.
       *
       * @return index of the leaf.
       **/
      uint32_t get_ltree_address() const
         {
         BOTAN_ASSERT(get_type() == Type::LTree_Address,
                      "set_ltree_address() requires XMSS_Address::Type::"
                      "LTree_Address.");
         return get_hi32(2);
         }

      /**
       * Sets the index of the leaf computed with this LTree. A call to this
       * method is only valid, if the address type is set to
       * Type::LTree_Address.
       *
       * @param value index of the leaf.
       **/
      void set_ltree_address(uint32_t value)
         {
         BOTAN_ASSERT(get_type() == Type::LTree_Address,
                      "set_ltree_address() requires XMSS_Address::Type::"
                      "LTree_Address.");
         set_hi32(2, value);
         }

      /**
       * Retrieve the chain address. A call to this method is only valid, if
       * the address type is set to Type::OTS_Hash_Address.
       *
       * @return chain address.
       **/
      uint32_t get_chain_address() const
         {
         BOTAN_ASSERT(get_type() == Type::OTS_Hash_Address,
                      "get_chain_address() requires XMSS_Address::Type::"
                      "OTS_Hash_Address.");
         return get_lo32(2);
         }

      /**
       * Set the chain address. A call to this method is only valid, if
       * the address type is set to Type::OTS_Hash_Address.
       **/
      void set_chain_address(uint32_t value)
         {
         BOTAN_ASSERT(get_type() == Type::OTS_Hash_Address,
                      "set_chain_address() requires XMSS_Address::Type::"
                      "OTS_Hash_Address.");
         set_lo32(2, value);
         }

      /**
       * Retrieves the height of the tree node to be computed within the
       * tree. A call to this method is only valid, if the address type is
       * set to Type::LTree_Address or Type::Hash_Tree_Address.
       *
       * @return height of the tree node.
       **/
      uint32_t get_tree_height() const
         {
         BOTAN_ASSERT(get_type() == Type::LTree_Address ||
                      get_type() == Type::Hash_Tree_Address,
                      "get_tree_height() requires XMSS_Address::Type::"
                      "LTree_Address or XMSS_Address::Type::Hash_Tree_Address.");
         return get_lo32(2);
         }

      /**
       * Sets the height of the tree node to be computed within the
       * tree. A call to this method is only valid, if the address type is
       * set to Type::LTree_Address or Type::Hash_Tree_Address.
       *
       * @param value height of the tree node.
       **/
      void set_tree_height(uint32_t value)
         {
         BOTAN_ASSERT(get_type() == Type::LTree_Address ||
                      get_type() == Type::Hash_Tree_Address,
                      "set_tree_height() requires XMSS_Address::Type::"
                      "LTree_Address or XMSS_Address::Type::Hash_Tree_Address.");
         set_lo32(2, value);
         }

      /**
       * Retrieves the address of the hash function call within the chain.
       * A call to this method is only valid, if the address type is
       * set to Type::OTS_Hash_Address.
       *
       * @return address of the hash function call within chain.
       **/
      uint32_t get_hash_address() const
         {
         BOTAN_ASSERT(get_type() == Type::OTS_Hash_Address,
                      "get_hash_address() requires XMSS_Address::Type::"
                      "OTS_Hash_Address.");
         return get_hi32(3);
         }

      /**
       * Sets the address of the hash function call within the chain.
       * A call to this method is only valid, if the address type is
       * set to Type::OTS_Hash_Address.
       *
       * @param value address of the hash function call within chain.
       **/
      void set_hash_address(uint32_t value)
         {
         BOTAN_ASSERT(get_type() == Type::OTS_Hash_Address,
                      "set_hash_address() requires XMSS_Address::Type::"
                      "OTS_Hash_Address.");
         set_hi32(3, value);
         }

      /**
       * Retrieves the index of the tree node at current tree height in the
       * tree. A call to this method is only valid, if the address type is
       * set to Type::LTree_Address or Type::Hash_Tree_Address.
       *
       * @return index of the tree node at current height.
       **/
      uint32_t get_tree_index() const
         {
         BOTAN_ASSERT(get_type() == Type::LTree_Address ||
                      get_type() == Type::Hash_Tree_Address,
                      "get_tree_index() requires XMSS_Address::Type::"
                      "LTree_Address or XMSS_Address::Type::Hash_Tree_Address.");
         return get_hi32(3);
         }

      /**
       * Sets the index of the tree node at current tree height in the
       * tree. A call to this method is only valid, if the address type is
       * set to Type::LTree_Address or Type::Hash_Tree_Address.
       *
       * @param value index of the tree node at current height.
       **/
      void set_tree_index(uint32_t value)
         {
         BOTAN_ASSERT(get_type() == Type::LTree_Address ||
                      get_type() == Type::Hash_Tree_Address,
                      "set_tree_index() requires XMSS_Address::Type::"
                      "LTree_Address or XMSS_Address::Type::Hash_Tree_Address.");
         set_hi32(3, value);
         }

      const secure_vector<uint8_t>& bytes() const
         {
         return m_data;
         }

      secure_vector<uint8_t>& bytes()
         {
         return m_data;
         }

      /**
       * @return the size of an XMSS_Address
       **/
      size_t size() const
         {
         return m_data.size();
         }

      XMSS_Address()
         : m_data(m_address_size)
         {
         set_type(Type::None);
         }

      XMSS_Address(Type type)
         : m_data(m_address_size)
         {
         set_type(type);
         }

      XMSS_Address(const secure_vector<uint8_t>& data) : m_data(data)
         {
         BOTAN_ASSERT(m_data.size() == m_address_size,
                      "XMSS_Address must be of 256 bits size.");
         }

      XMSS_Address(secure_vector<uint8_t>&& data) : m_data(std::move(data))
         {
         BOTAN_ASSERT(m_data.size() == m_address_size,
                      "XMSS_Address must be of 256 bits size.");
         }

   protected:
      secure_vector<uint8_t> m_data;

   private:
      static const size_t m_address_size = 32;

      inline uint32_t get_hi32(size_t offset) const
         {
         return ((0x000000FF & m_data[8 * offset + 3])       |
                 (0x000000FF & m_data[8 * offset + 2]) <<  8 |
                 (0x000000FF & m_data[8 * offset + 1]) << 16 |
                 (0x000000FF & m_data[8 * offset    ]) << 24);
         }

      inline void set_hi32(size_t offset, uint32_t value)
         {
         m_data[offset * 8    ] = ((value >> 24) & 0xFF);
         m_data[offset * 8 + 1] = ((value >> 16) & 0xFF);
         m_data[offset * 8 + 2] = ((value >>  8) & 0xFF);
         m_data[offset * 8 + 3] = ((value      ) & 0xFF);
         }

      inline uint32_t get_lo32(size_t offset) const
         {
         return ((0x000000FF & m_data[8 * offset + 7])       |
                 (0x000000FF & m_data[8 * offset + 6]) <<  8 |
                 (0x000000FF & m_data[8 * offset + 5]) << 16 |
                 (0x000000FF & m_data[8 * offset + 4]) << 24);
         }

      inline void set_lo32(size_t offset, uint32_t value)
         {
         m_data[offset * 8 + 4] = ((value >> 24) & 0xFF);
         m_data[offset * 8 + 5] = ((value >> 16) & 0xFF);
         m_data[offset * 8 + 6] = ((value >>  8) & 0xFF);
         m_data[offset * 8 + 7] = ((value      ) & 0xFF);
         }
   };

}

#endif
