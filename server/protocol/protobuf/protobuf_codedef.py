#!/bin/python
#-*- encoding:UTF-8 -*-  

import os
import sys 

def delBack(s, sub):
    l1 = len(s)
    l2 = len(sub)

    if l1 < l2 :
        return s

    back = s[ -l2 : ] 
    if cmp(back, sub) == 0:
        return s[: -l2]
    return s

def delFront(s, sub):
    l1 = len(s)
    l2 = len(sub)

    if l1 < l2 :
        return s

    front = s[: l2] 
    if cmp(front, sub) == 0 : 
        return s[l2 :]
    return s

search_packet_key = ("namespace ")  #空格不能去
search_message_key = ("extern const uint32_t")
message_code = 0

#sys.argv[1]	config文件夹路径 
#sys.argv[2]	生成cpp 或 xml
#sys.argv[3]	外部消息或内部消息 public 或 private

out_put_dir = sys.argv[1]
file_suffix = sys.argv[2]
hpp_folder = sys.argv[3]

protobuf_or_rawmsg = ""
public_or_private = ""
if "rawmsg" in os.getcwd():
    protobuf_or_rawmsg = "RAWMSG"
elif "protobuf" in os.getcwd():
    protobuf_or_rawmsg = "PROTOBUF"

if "public" == hpp_folder:
    public_or_private = "PUBLIC"
elif "private" == hpp_folder:
    public_or_private = "PRIVATE"

if "PROTOBUF" == protobuf_or_rawmsg and "PUBLIC" == public_or_private:
    message_code = 0x00000000 | 0x00000000
elif "PROTOBUF" == protobuf_or_rawmsg and "PRIVATE" == public_or_private:
    message_code = 0x00000000 | 0x40000000
elif "RAWMSG" == protobuf_or_rawmsg and "PUBLIC" == public_or_private:
    message_code = 0x10000000 | 0x00000000
elif "RAWMSG" == protobuf_or_rawmsg and "PRIVATE" == public_or_private:
    message_code = 0x10000000 | 0x40000000


#过滤文件 *.codedef.private.h
codedef_files_list = []
def anyTrue(predicate, sequence):
    return True in map(predicate, sequence)

def filterFiles(folder, exts):
    for fileName in os.listdir(folder):
        if os.path.isfile(folder + '/' + fileName) :
            if not anyTrue(fileName.endswith, exts):
                continue
            codedef_files_list.append(fileName)

exts = ['.codedef.h']
msg_def_dir = os.getcwd() + '/' + hpp_folder
filterFiles(msg_def_dir, exts)


#包含头文件
def getCppHeader(fileName):
    headerName = \
            '#include ' + '"' + fileName + '"' + "\n" + \
            '#include ' + '"' + fileName.split(".")[0] + '.pb.h"' "\n"
    return headerName

#获取包含头文件 #include "login.h"
def getCppHeaderStr():
    cpp_header_str=""
    for fileName in codedef_files_list:
        cpp_header_str = cpp_header_str + getCppHeader(fileName)

    cpp_header_str = cpp_header_str + '\n'
    return cpp_header_str

def getMsgCodeName(search_line):
    msg_name = delFront(search_line, "extern")	 
    msg_name = msg_name.strip(";")  
    msg_name = msg_name.strip()		#去除空格
    return msg_name

def getFullMsgName(search_line):
    msg_name = delFront(search_line, "extern const uint32_t code")	 
    msg_name = msg_name.strip(";")  
    msg_name = msg_name.strip()		#去除空格
    return msg_name

#获取命名空间名称
def getNamespaceStr():
    for fileName in codedef_files_list:
        fileName = msg_def_dir + '/' + fileName
        read_file_handler = open(fileName, 'r')
        read_text_list = read_file_handler.readlines()  
        for line in read_text_list:
            search_line = line.strip()	#去除前面空格
            if search_line.find(search_packet_key) == 0:   # 查找namespace只有一个，找到就break
                space_str = delFront(search_line, search_packet_key)
                space_str = space_str.strip("{")
                space_str = space_str.strip(";")

                read_file_handler.close()
                return space_str


#***************************** 仅生成一个 *.xml 或 *.cpp 消息号文件 ********************************
if "cpp" == file_suffix:
    namespace_str = getNamespaceStr()
    cpp_namespace_start_str="namespace " + namespace_str + "\n" + "{" + "\n"
    cpp_namespace_end_str="}"

    access_type_str = "public"
    if public_or_private == "PRIVATE":
        access_type_str = "private"

    codedef_file_name = out_put_dir + "/codedef." + access_type_str + ".cpp"
    cpp_start_str = getCppHeaderStr()
    write_cpp_handler = open(codedef_file_name, 'w')
    write_cpp_handler.write(cpp_start_str)
    write_cpp_handler.write(cpp_namespace_start_str)

    for fileName in codedef_files_list:
        fileName = msg_def_dir + '/' + fileName
        read_file_handler = open(fileName, 'r')
        read_text_list = read_file_handler.readlines()  

        for line in read_text_list:
            search_line = line.strip()  #去除前面的空格
            if search_line.find(search_message_key) == 0:		# 查找extern
                message_code += 1
                msg_code_name = getMsgCodeName(search_line)
                msg_code_value_assign = "    " + msg_code_name + "\t\t= " + `message_code` + ";\n"
                write_cpp_handler.write(msg_code_value_assign)


        for line in read_text_list:
            search_line = line.strip()  #去除前面的空格
            if search_line.find(search_message_key) == 0:		# 查找extern
                msg_name = getMsgCodeName(search_line)[19:]
                msg_obj_define = "    const " + msg_name + "\t\t temp" + msg_name + ";\n"
                write_cpp_handler.write(msg_obj_define)

        print "cpp, Parse " + fileName + " OK!"
        read_file_handler.close()	#关闭文件句柄	自定义的原始消息号头文件

    write_cpp_handler.write(cpp_namespace_end_str)
    write_cpp_handler.close()	#关闭文件句柄	写protobuf.codedef.private.cpp 消息号

elif "xml" == file_suffix:
    xml_start_str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>\n"
    xml_end_str = "</root>"
    xml_file_name = out_put_dir + "/protobuf.codedef.xml"	#protobuf.codedef.xml 
    write_xml_hanlder = open(xml_file_name, 'w')
    write_xml_hanlder.write(xml_start_str)

    namespace_str = getNamespaceStr()

    for fileName in codedef_files_list:
        fileName = msg_def_dir + '/' + fileName
        read_file_handler = open(fileName, 'r')
        read_text_list = read_file_handler.readlines()  

        for line in read_text_list:
            search_line = line.strip()  #去除前面的空格
            if search_line.find(search_message_key) == 0:		# 查找extern
                message_code += 1

                xml_msg_name = getFullMsgName(search_line)
                xml_msg = \
                        '\t<item msg_code="' + `message_code` + '"'\
                        ' msg_name="' + namespace_str + "." + xml_msg_name + '"/>\n'  
                write_xml_hanlder.write(xml_msg)

        print "Xml, Parse " + fileName + " OK!"
        read_file_handler.close()	#关闭文件句柄	自定义的原始消息号头文件

    write_xml_hanlder.write(xml_end_str)
    write_xml_hanlder.close()	#关闭文件句柄	写protobuf.codedef.private.xml消息号
elif "h" == file_suffix:
    access_type_str = "public"
    if public_or_private == "PRIVATE":
        access_type_str = "private"
    all_in_one_codedef_header_file_name = out_put_dir + "/codedef." + access_type_str + ".h"
    write_header_file_handler = open(all_in_one_codedef_header_file_name, 'w')
    cpp_macro = "PROTOCOL_" + public_or_private + "_CODEDEF_HPP\n"
    write_header_file_handler.write("#ifndef " + cpp_macro)
    write_header_file_handler.write("#define " + cpp_macro)
    write_header_file_handler.write(getCppHeaderStr())
    write_header_file_handler.write("#endif\n")
    write_header_file_handler.close()
elif "json" == file_suffix:
    access_type_str = "public"
    if public_or_private == "PRIVATE":
        access_type_str = "private"
    json_file_name = out_put_dir + "/protobuf.codedef.json"
    json_file_hanlder = open(json_file_name, "w")
    json_file_hanlder.write("{\n")
    namespace_str = getNamespaceStr()
    is_first_item = True
    for fileName in codedef_files_list:
        fileName = msg_def_dir + '/' + fileName
        read_file_handler = open(fileName, 'r')
        read_text_list = read_file_handler.readlines()  

        for line in read_text_list:
            search_line = line.strip()  #去除前面的空格
            if search_line.find(search_message_key) == 0:		# 查找extern
                message_code += 1

                full_msg_name = getFullMsgName(search_line)
                new_line = '\t"{}.{}": {},\n\t"{}": "{}.{}"'.format(
                        namespace_str, full_msg_name, message_code,
                        message_code, namespace_str, full_msg_name)
                if is_first_item :
                    is_first_item = False
                else:
                    new_line = ",\n" + new_line
                json_file_hanlder.write(new_line)
        read_file_handler.close()	#关闭文件句柄	自定义的原始消息号头文件
    json_file_hanlder.write('\n}\n')
    json_file_hanlder.close()

#    access_type_str = "public"
#    if public_or_private == "PRIVATE":
#        access_type_str = "private"
#
#    all_in_one_codedef_header_file_name = out_put_dir + "/codedef." + access_type_str + ".h"
#    write_header_file_handler = open(all_in_one_codedef_header_file_name, 'w')
#    cpp_macro = "PROTOCOL_" + public_or_private + "_CODEDEF_HPP\n"
#    write_header_file_handler.write("#ifndef " + cpp_macro)
#    write_header_file_handler.write("#define " + cpp_macro)
#    for fileName in codedef_files_list:
#        fileName = "access_type_str" + '/' + fileName
#        newLine = '#include "' + fileName + '"\n'
#        write_header_file_handler.write(newLine)
#    write_header_file_handler.write("#endif\n")
#    write_header_file_handler.close()
#    return

