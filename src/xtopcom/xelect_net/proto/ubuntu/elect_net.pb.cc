// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: elect_net.proto

#include "elect_net.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
namespace top {
namespace elect {
namespace protobuf {
class VhostMessageDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<VhostMessage> _instance;
} _VhostMessage_default_instance_;
}  // namespace protobuf
}  // namespace elect
}  // namespace top
static void InitDefaultsscc_info_VhostMessage_elect_5fnet_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::top::elect::protobuf::_VhostMessage_default_instance_;
    new (ptr) ::top::elect::protobuf::VhostMessage();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
  ::top::elect::protobuf::VhostMessage::InitAsDefaultInstance();
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_VhostMessage_elect_5fnet_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 0, 0, InitDefaultsscc_info_VhostMessage_elect_5fnet_2eproto}, {}};

static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_elect_5fnet_2eproto[1];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_elect_5fnet_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_elect_5fnet_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_elect_5fnet_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::top::elect::protobuf::VhostMessage, _has_bits_),
  PROTOBUF_FIELD_OFFSET(::top::elect::protobuf::VhostMessage, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::top::elect::protobuf::VhostMessage, type_),
  PROTOBUF_FIELD_OFFSET(::top::elect::protobuf::VhostMessage, data_),
  PROTOBUF_FIELD_OFFSET(::top::elect::protobuf::VhostMessage, cb_type_),
  PROTOBUF_FIELD_OFFSET(::top::elect::protobuf::VhostMessage, node_id_),
  2,
  0,
  3,
  1,
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 9, sizeof(::top::elect::protobuf::VhostMessage)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::top::elect::protobuf::_VhostMessage_default_instance_),
};

const char descriptor_table_protodef_elect_5fnet_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\017elect_net.proto\022\022top.elect.protobuf\"L\n"
  "\014VhostMessage\022\014\n\004type\030\001 \001(\r\022\014\n\004data\030\002 \001("
  "\014\022\017\n\007cb_type\030\003 \001(\r\022\017\n\007node_id\030\004 \001(\014"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_elect_5fnet_2eproto_deps[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase*const descriptor_table_elect_5fnet_2eproto_sccs[1] = {
  &scc_info_VhostMessage_elect_5fnet_2eproto.base,
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_elect_5fnet_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_elect_5fnet_2eproto = {
  false, false, descriptor_table_protodef_elect_5fnet_2eproto, "elect_net.proto", 115,
  &descriptor_table_elect_5fnet_2eproto_once, descriptor_table_elect_5fnet_2eproto_sccs, descriptor_table_elect_5fnet_2eproto_deps, 1, 0,
  schemas, file_default_instances, TableStruct_elect_5fnet_2eproto::offsets,
  file_level_metadata_elect_5fnet_2eproto, 1, file_level_enum_descriptors_elect_5fnet_2eproto, file_level_service_descriptors_elect_5fnet_2eproto,
};

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_elect_5fnet_2eproto = (static_cast<void>(::PROTOBUF_NAMESPACE_ID::internal::AddDescriptors(&descriptor_table_elect_5fnet_2eproto)), true);
namespace top {
namespace elect {
namespace protobuf {

// ===================================================================

void VhostMessage::InitAsDefaultInstance() {
}
class VhostMessage::_Internal {
 public:
  using HasBits = decltype(std::declval<VhostMessage>()._has_bits_);
  static void set_has_type(HasBits* has_bits) {
    (*has_bits)[0] |= 4u;
  }
  static void set_has_data(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
  static void set_has_cb_type(HasBits* has_bits) {
    (*has_bits)[0] |= 8u;
  }
  static void set_has_node_id(HasBits* has_bits) {
    (*has_bits)[0] |= 2u;
  }
};

VhostMessage::VhostMessage(::PROTOBUF_NAMESPACE_ID::Arena* arena)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena) {
  SharedCtor();
  RegisterArenaDtor(arena);
  // @@protoc_insertion_point(arena_constructor:top.elect.protobuf.VhostMessage)
}
VhostMessage::VhostMessage(const VhostMessage& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      _has_bits_(from._has_bits_) {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  data_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  if (from._internal_has_data()) {
    data_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from._internal_data(),
      GetArena());
  }
  node_id_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  if (from._internal_has_node_id()) {
    node_id_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from._internal_node_id(),
      GetArena());
  }
  ::memcpy(&type_, &from.type_,
    static_cast<size_t>(reinterpret_cast<char*>(&cb_type_) -
    reinterpret_cast<char*>(&type_)) + sizeof(cb_type_));
  // @@protoc_insertion_point(copy_constructor:top.elect.protobuf.VhostMessage)
}

void VhostMessage::SharedCtor() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&scc_info_VhostMessage_elect_5fnet_2eproto.base);
  data_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  node_id_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  ::memset(&type_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&cb_type_) -
      reinterpret_cast<char*>(&type_)) + sizeof(cb_type_));
}

VhostMessage::~VhostMessage() {
  // @@protoc_insertion_point(destructor:top.elect.protobuf.VhostMessage)
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

void VhostMessage::SharedDtor() {
  GOOGLE_DCHECK(GetArena() == nullptr);
  data_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  node_id_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

void VhostMessage::ArenaDtor(void* object) {
  VhostMessage* _this = reinterpret_cast< VhostMessage* >(object);
  (void)_this;
}
void VhostMessage::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void VhostMessage::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const VhostMessage& VhostMessage::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_VhostMessage_elect_5fnet_2eproto.base);
  return *internal_default_instance();
}


void VhostMessage::Clear() {
// @@protoc_insertion_point(message_clear_start:top.elect.protobuf.VhostMessage)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    if (cached_has_bits & 0x00000001u) {
      data_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000002u) {
      node_id_.ClearNonDefaultToEmpty();
    }
  }
  if (cached_has_bits & 0x0000000cu) {
    ::memset(&type_, 0, static_cast<size_t>(
        reinterpret_cast<char*>(&cb_type_) -
        reinterpret_cast<char*>(&type_)) + sizeof(cb_type_));
  }
  _has_bits_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* VhostMessage::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  ::PROTOBUF_NAMESPACE_ID::Arena* arena = GetArena(); (void)arena;
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // optional uint32 type = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 8)) {
          _Internal::set_has_type(&has_bits);
          type_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // optional bytes data = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 18)) {
          auto str = _internal_mutable_data();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // optional uint32 cb_type = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 24)) {
          _Internal::set_has_cb_type(&has_bits);
          cb_type_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // optional bytes node_id = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 34)) {
          auto str = _internal_mutable_node_id();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag,
            _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
            ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  _has_bits_.Or(has_bits);
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* VhostMessage::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:top.elect.protobuf.VhostMessage)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  // optional uint32 type = 1;
  if (cached_has_bits & 0x00000004u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteUInt32ToArray(1, this->_internal_type(), target);
  }

  // optional bytes data = 2;
  if (cached_has_bits & 0x00000001u) {
    target = stream->WriteBytesMaybeAliased(
        2, this->_internal_data(), target);
  }

  // optional uint32 cb_type = 3;
  if (cached_has_bits & 0x00000008u) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteUInt32ToArray(3, this->_internal_cb_type(), target);
  }

  // optional bytes node_id = 4;
  if (cached_has_bits & 0x00000002u) {
    target = stream->WriteBytesMaybeAliased(
        4, this->_internal_node_id(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:top.elect.protobuf.VhostMessage)
  return target;
}

size_t VhostMessage::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:top.elect.protobuf.VhostMessage)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  if (cached_has_bits & 0x0000000fu) {
    // optional bytes data = 2;
    if (cached_has_bits & 0x00000001u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
          this->_internal_data());
    }

    // optional bytes node_id = 4;
    if (cached_has_bits & 0x00000002u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
          this->_internal_node_id());
    }

    // optional uint32 type = 1;
    if (cached_has_bits & 0x00000004u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::UInt32Size(
          this->_internal_type());
    }

    // optional uint32 cb_type = 3;
    if (cached_has_bits & 0x00000008u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::UInt32Size(
          this->_internal_cb_type());
    }

  }
  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void VhostMessage::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:top.elect.protobuf.VhostMessage)
  GOOGLE_DCHECK_NE(&from, this);
  const VhostMessage* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<VhostMessage>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:top.elect.protobuf.VhostMessage)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:top.elect.protobuf.VhostMessage)
    MergeFrom(*source);
  }
}

void VhostMessage::MergeFrom(const VhostMessage& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:top.elect.protobuf.VhostMessage)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._has_bits_[0];
  if (cached_has_bits & 0x0000000fu) {
    if (cached_has_bits & 0x00000001u) {
      _internal_set_data(from._internal_data());
    }
    if (cached_has_bits & 0x00000002u) {
      _internal_set_node_id(from._internal_node_id());
    }
    if (cached_has_bits & 0x00000004u) {
      type_ = from.type_;
    }
    if (cached_has_bits & 0x00000008u) {
      cb_type_ = from.cb_type_;
    }
    _has_bits_[0] |= cached_has_bits;
  }
}

void VhostMessage::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:top.elect.protobuf.VhostMessage)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void VhostMessage::CopyFrom(const VhostMessage& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:top.elect.protobuf.VhostMessage)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool VhostMessage::IsInitialized() const {
  return true;
}

void VhostMessage::InternalSwap(VhostMessage* other) {
  using std::swap;
  _internal_metadata_.Swap<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(&other->_internal_metadata_);
  swap(_has_bits_[0], other->_has_bits_[0]);
  data_.Swap(&other->data_, &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
  node_id_.Swap(&other->node_id_, &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(VhostMessage, cb_type_)
      + sizeof(VhostMessage::cb_type_)
      - PROTOBUF_FIELD_OFFSET(VhostMessage, type_)>(
          reinterpret_cast<char*>(&type_),
          reinterpret_cast<char*>(&other->type_));
}

::PROTOBUF_NAMESPACE_ID::Metadata VhostMessage::GetMetadata() const {
  return GetMetadataStatic();
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace protobuf
}  // namespace elect
}  // namespace top
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::top::elect::protobuf::VhostMessage* Arena::CreateMaybeMessage< ::top::elect::protobuf::VhostMessage >(Arena* arena) {
  return Arena::CreateMessageInternal< ::top::elect::protobuf::VhostMessage >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
