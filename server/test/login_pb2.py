# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: login.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='login.proto',
  package='PublicProto',
  serialized_pb=_b('\n\x0blogin.proto\x12\x0bPublicProto\"\x1f\n\x07testmsg\x12\t\n\x01\x61\x18\x01 \x02(\x05\x12\t\n\x01\x62\x18\x02 \x02(\x05\"*\n\x17\x41\x63\x63ountAndTokenToServer\x12\x0f\n\x07\x61\x63\x63ount\x18\x01 \x02(\t')
)
_sym_db.RegisterFileDescriptor(DESCRIPTOR)




_TESTMSG = _descriptor.Descriptor(
  name='testmsg',
  full_name='PublicProto.testmsg',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='a', full_name='PublicProto.testmsg.a', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='b', full_name='PublicProto.testmsg.b', index=1,
      number=2, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=28,
  serialized_end=59,
)


_ACCOUNTANDTOKENTOSERVER = _descriptor.Descriptor(
  name='AccountAndTokenToServer',
  full_name='PublicProto.AccountAndTokenToServer',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='account', full_name='PublicProto.AccountAndTokenToServer.account', index=0,
      number=1, type=9, cpp_type=9, label=2,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=61,
  serialized_end=103,
)

DESCRIPTOR.message_types_by_name['testmsg'] = _TESTMSG
DESCRIPTOR.message_types_by_name['AccountAndTokenToServer'] = _ACCOUNTANDTOKENTOSERVER

testmsg = _reflection.GeneratedProtocolMessageType('testmsg', (_message.Message,), dict(
  DESCRIPTOR = _TESTMSG,
  __module__ = 'login_pb2'
  # @@protoc_insertion_point(class_scope:PublicProto.testmsg)
  ))
_sym_db.RegisterMessage(testmsg)

AccountAndTokenToServer = _reflection.GeneratedProtocolMessageType('AccountAndTokenToServer', (_message.Message,), dict(
  DESCRIPTOR = _ACCOUNTANDTOKENTOSERVER,
  __module__ = 'login_pb2'
  # @@protoc_insertion_point(class_scope:PublicProto.AccountAndTokenToServer)
  ))
_sym_db.RegisterMessage(AccountAndTokenToServer)


# @@protoc_insertion_point(module_scope)
