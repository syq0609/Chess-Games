import login_pb2 as loginProto
import struct


testmsg1 = loginProto.testmsg()
testmsg1.a = 101
testmsg1.b = 202

print testmsg1

print "ByteSize:",testmsg1.ByteSize()
binData = testmsg1.SerializeToString()

print "len(binData):",len(binData)

testmsg2 = loginProto.testmsg()
testmsg2.ParseFromString(binData)

print testmsg2

print "----------------------------------"

for b in bytearray(binData):
    buf = struct.pack("x", b)

print len(buf)
