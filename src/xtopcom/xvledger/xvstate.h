// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvproperty.h"
#include "xvblock.h"

namespace top
{
    namespace base
    {
        //xvbstate_t(block state) present all properties of one block(specified block height)
        //note: it is not allow to modify anymore after xvbstate_t object is generated from serialize or new(initialize by subclass)
        class xvblock_t;
        class xvbstate_t : public xvexegroup_t
        {
            friend class xvblock_t;
            friend class xvblockstore_t;
            friend class xvstatestore_t;
            typedef xvexegroup_t base;

        public:
            static  const std::string   name(){ return std::string("xvbstat");}
            virtual std::string         get_obj_name() const override {return name();}
            static  const std::string   make_unit_name(const std::string & account, const uint64_t blockheight);
        private:
            static  void  register_object(xcontext_t & context);
            enum{enum_obj_type = enum_xobject_type_vbstate};//allow xbase create xvstate_t object from xdataobj_t::read_from()
            enum{enum_max_property_count = 256}; //only allow 256 properties for each account
            
        public:
            xvbstate_t(xvblock_t& for_block,xvexeunit_t * parent_unit = NULL,enum_xdata_type type = (enum_xdata_type)enum_xobject_type_vbstate);
            
            //debug & ut-test only
            xvbstate_t(const std::string & account,const uint64_t block_height,const uint64_t block_viewid,const std::string & last_block_hash,const std::string &last_full_block_hash,const uint64_t last_full_block_height, const uint32_t raw_block_versions,const uint16_t raw_block_types, xvexeunit_t * parent_unit = NULL);
            
        protected:
            xvbstate_t(enum_xdata_type type = (enum_xdata_type)enum_xobject_type_vbstate);
            xvbstate_t(const xvbstate_t & obj);
            virtual ~xvbstate_t();
            
        private://not implement those private construction
            xvbstate_t(xvbstate_t &&);
            xvbstate_t & operator = (const xvbstate_t & other);

        public:
            virtual xvexeunit_t*  clone() override; //cone a new object with same state //each property is readonly after clone
                       
            virtual std::string   dump() const override;  //just for debug purpose
            virtual void*         query_interface(const int32_t _enum_xobject_type_) override;//caller need to cast (void*) to related ptr
            //clear canvas assocaited with vbstate and properties,all recored instruction are clear as well
            //note:reset_canvas not modify the actua state of properties/block, it just against for instrution on canvas
            virtual bool          reset_canvas();

        public:
            //bin-log related functions
            enum_xerror_code      encode_change_to_binlog(std::string & output_bin);//
            enum_xerror_code      encode_change_to_binlog(xstream_t & output_bin);//
            //note:rebase_change_to_snapshot rebase all changes into properties and take snapshot,then clean all previsouse changes,useful to generate full block of full-state
            virtual bool          rebase_change_to_snapshot(); //snapshot for whole xvbstate of every properties
            
            enum_xerror_code      decode_change_from_binlog(const std::string & from_bin_log,std::deque<top::base::xvmethod_t> & out_records);
            enum_xerror_code      decode_change_from_binlog(xstream_t & from_bin_log,const uint32_t bin_log_size,std::deque<top::base::xvmethod_t> & out_records);
            
            bool                  apply_changes_of_binlog(const std::string & from_bin_log);//apply changes to current states,use carefully
            virtual bool          apply_changes_of_binlog(xstream_t & from_bin_log,const uint32_t bin_log_size);//apply changes to current states,use carefully
            
        public://read-only
            inline const std::string&   get_account_addr()          const {return m_account_addr;}
            inline const uint64_t       get_block_height()          const {return m_block_height;}
            inline const uint64_t       get_block_viewid()          const {return m_block_viewid;}
            
            inline const std::string &  get_last_block_hash()       const {return m_last_block_hash;}
            inline const std::string &  get_last_fullblock_hash()   const {return m_last_full_block_hash;}
            inline const uint64_t       get_last_fullblock_height() const {return m_last_full_block_height;}
            
            const int                   get_block_level() const;
            const int                   get_block_class() const;
            const int                   get_block_type()  const;
            
            inline uint32_t             get_block_features()      const  { return (m_block_versions >> 24); }
            inline uint32_t             get_block_version()       const  { return (m_block_versions & 0x00FFFFFF);}
            inline int                  get_block_version_major() const  { return ((m_block_versions& 0x00FF0000) >> 16);}
            inline int                  get_block_version_minor() const  { return ((m_block_versions& 0x0000FF00) >> 8);}
            inline int                  get_block_version_patch() const  { return ((m_block_versions& 0x000000FF));}
            
            bool                        find_property(const std::string & property_name); //check whether property already existing
        
        public://note: only allow access by our kernel module. it means private for application'contract
            xauto_ptr<xtokenvar_t>              load_token_var(const std::string & property_name);//for main token(e.g. TOP Token)
            xauto_ptr<xnoncevar_t>              load_nonce_var(const std::string & property_name);//for noance of account
            xauto_ptr<xcodevar_t>               load_code_var(const std::string & property_name); //for contract code of account
            xauto_ptr<xmtokens_t>               load_multiple_tokens_var(const std::string & property_name);//for native tokens
            xauto_ptr<xmkeys_t>                 load_multiple_keys_var(const std::string & property_name); //to manage pubkeys of account
            
        public: //general ones for both kernel and application
            xauto_ptr<xstringvar_t>             load_string_var(const std::string & property_name);
            xauto_ptr<xhashmapvar_t>            load_hashmap_var(const std::string & property_name);
            xauto_ptr<xvproperty_t>             load_property(const std::string & property_name);//general way
            
        public://load function of integer  for both kernel and application
            xauto_ptr<xvintvar_t<int64_t>>      load_int64_var(const std::string & property_name);
            xauto_ptr<xvintvar_t<uint64_t>>     load_uint64_var(const std::string & property_name);
            
        public://load function of deque  for both kernel and application
            xauto_ptr<xdequevar_t<int8_t>>      load_int8_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<int16_t>>     load_int16_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<int32_t>>     load_int32_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<int64_t>>     load_int64_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<uint64_t>>    load_uint64_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<std::string>> load_string_deque_var(const std::string & property_name);
            
        public://load function of map  for both kernel and application
            xauto_ptr<xmapvar_t<int8_t>>        load_int8_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<int16_t>>       load_int16_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<int32_t>>       load_int32_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<int64_t>>       load_int64_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<uint64_t>>      load_uint64_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<std::string>>   load_string_map_var(const std::string & property_name);
            
        public://note: only allow access by our kernel. it means private for application'contract
            xauto_ptr<xtokenvar_t>              new_token_var(const std::string & property_name);//for main token(e.g. TOP Token)
            xauto_ptr<xnoncevar_t>              new_nonce_var(const std::string & property_name);//for noance of account
            xauto_ptr<xcodevar_t>               new_code_var(const std::string & property_name); //for contract code of account
            xauto_ptr<xmtokens_t>               new_multiple_tokens_var(const std::string & property_name);//for native tokens
            xauto_ptr<xmkeys_t>                 new_multiple_keys_var(const std::string & property_name); //to manage pubkeys of account
            
        public://for general ones for both kernel and application
            xauto_ptr<xstringvar_t>             new_string_var(const std::string & property_name);
            xauto_ptr<xhashmapvar_t>            new_hashmap_var(const std::string & property_name);
           
        public://integer related  for both kernel and application
            xauto_ptr<xvintvar_t<int64_t>>      new_int64_var(const std::string & property_name);
            xauto_ptr<xvintvar_t<uint64_t>>     new_uint64_var(const std::string & property_name);
            
        public://new function of deque  for both kernel and application
            xauto_ptr<xdequevar_t<int8_t>>      new_int8_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<int16_t>>     new_int16_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<int32_t>>     new_int32_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<int64_t>>     new_int64_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<uint64_t>>    new_uint64_deque_var(const std::string & property_name);
            xauto_ptr<xdequevar_t<std::string>> new_string_deque_var(const std::string & property_name);
            
        public://new function of map  for both kernel and application
            xauto_ptr<xmapvar_t<int8_t>>        new_int8_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<int16_t>>       new_int16_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<int32_t>>       new_int32_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<int64_t>>       new_int64_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<uint64_t>>      new_uint64_map_var(const std::string & property_name);
            xauto_ptr<xmapvar_t<std::string>>   new_string_map_var(const std::string & property_name);
            
        protected: //internal use only
            bool      reset_property(const std::string & property_name,const xvalue_t & property_value);
            bool      renew_property(const std::string & property_name,const int property_type,const xvalue_t & property_value);
            bool      del_property(const std::string & property_name);
            
        protected:
            //subclass extend behavior and load more information instead of a raw one
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t         do_write(xstream_t & stream) override;//allow subclass extend behavior
            virtual int32_t         do_read(xstream_t & stream)  override;//allow subclass extend behavior
            
            virtual std::string     get_property_value(const std::string & name);
            virtual xvproperty_t*   get_property_object(const std::string & name);
        
            bool                    clone_properties_from(xvbstate_t& source);//note: just only clone the state of properties
            enum_xerror_code        encode_change_to_binlog(xvcanvas_t* source_canvas,std::string & output_bin);

        private:
            std::string     m_account_addr;
            uint64_t        m_block_height;
            uint64_t        m_block_viewid;
            std::string     m_last_block_hash;       //point the last block'hash
            std::string     m_last_full_block_hash;  //any block need carry the m_last_full_block_hash
            uint64_t        m_last_full_block_height;//height of m_last_full_block
            
            uint32_t        m_block_versions; //version of chain as [8:features][8:major][8:minor][8:patch]
            //[1][enum_xvblock_class][enum_xvblock_level][enum_xvblock_type][enum_xvblock_reserved]
            uint16_t        m_block_types;
            
        private: //functions to modify value actually
            const xvalue_t  do_new_property(const xvmethod_t & op);
            const xvalue_t  do_reset_property(const xvmethod_t & op);
            const xvalue_t  do_renew_property(const xvmethod_t & op);
            const xvalue_t  do_del_property(const xvmethod_t & op);
            
            //keep private as safety
            xauto_ptr<xvproperty_t>  new_property(const std::string & property_name,const int propertyType);
            const xvalue_t  new_property_internal(const std::string & property_name,const int propertyType);
            xvmethod_t      renew_property_instruction(const std::string & property_name,const int  property_type,const xvalue_t & property_value);
            
        private://declare instruction methods supported by xvstate
            BEGIN_DECLARE_XVIFUNC_ID_API(enum_xvinstruct_class_state_function)
                IMPL_XVIFUNCE_ID_API(enum_xvinstruct_state_method_new_property,do_new_property)
                IMPL_XVIFUNCE_ID_API(enum_xvinstruct_state_method_reset_property,do_reset_property)
                IMPL_XVIFUNCE_ID_API(enum_xvinstruct_state_method_renew_property,do_renew_property)
                IMPL_XVIFUNCE_ID_API(enum_xvinstruct_state_method_del_property,do_del_property)
            END_DECLARE_XVIFUNC_ID_API(enum_xvinstruct_class_state_function)
        };
        
    }//end of namespace of base

}//end of namespace top
