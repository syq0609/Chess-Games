import socket
import struct
import ctypes
import login_pb2 as loginProto

ip = "127.0.0.1"
port = 8001
#s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#s.connect((ip, port))



testmsg = loginProto.testmsg()
testmsg.a = 101
testmsg.b = 202

print testmsg

print "ByteSize:",testmsg.ByteSize()
binData = testmsg.SerializeToString()

print "len(binData):",len(binData)


def msgToPacket(msgCode, msg):
    header = struct.pack("I", msgCode);
    body = msg.SerializeToString()
    return "".join(header + body)

#
#    headerRawSize = 4
#    packet = ctypes.create_string_buffer(0 + msg.ByteSize())
#    print repr(packet.raw)
#    offset = 0
#    struct.pack_into("1I", packet, offset, msgCode)
#    offset += headerRawSize
#    print repr(packet.raw)
#    body = msg.SerializeToString()
#    for byte in bytearray(body):
#        struct.pack_into("s", packet, offset, body)
#        offset += 1
#    return packet
    
def packetToMsg(packet):
    header = packet[0:4]
    body = packet[4:]

    msgCode = struct.unpack("I", header)
    msg = loginProto.testmsg()
    msg.ParseFromString(body)

    return msgCode[0],msg


packet = msgToPacket(1001, testmsg)
msgCode, msg = packetToMsg(packet)

print msgCode
print msg



