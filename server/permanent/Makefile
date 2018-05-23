PROTO_ROOT_DIR = $(shell pwd)

#google protobuf安装路径
PROTOBUF_INSTALLED_DIR = $(LIBS_DIR)/google_protobuf/installed

#安装好的protobuf编译器路径
PROTOBUF_COMPILER = $(PROTOBUF_INSTALLED_DIR)/bin/protoc

#protobuff定义文件存放目录
PB_DEFINE_DIR = $(PROTO_ROOT_DIR)/proto

objects = \
		  permanent_manager.o\


pb_objects = \
	proto/water13.pb.o\


CC = g++
cc = g++

cc_flags = $(CPP_STD_VER) -ggdb -Wall -Werror \
		   $(optimization_flag)\
		   -I$(PROTOBUF_INSTALLED_DIR)/include -I$(ROOT_DIR)

target_lib = libpermanent

.PHONY: all
all: $(target_lib).a 

#.a和.so依赖于objs
$(target_lib).a: $(pb_objects) $(objects)
	@echo $(pb_objects) $(objects)
	ar rs $(target_lib).a $(objects) $(pb_objects)

#objs 依赖于cpp 和 .pb.cc, 
$(pb_objects): %.pb.o: %.pb.cc
	$(cc) $(cc_flags) -c $< -o $@

$(objects): %.o: %.cpp
	$(cc) $(cc_flags) -c $< -o $@

#proto文件翻译成c++源文件
proto/%.pb.cc: proto/%.proto
	$(PROTOBUF_COMPILER) --proto_path=$(PB_DEFINE_DIR) --cpp_out=$(PB_DEFINE_DIR) $(PROTO_ROOT_DIR)/$<;
#	$(PROTOBUF_COMPILER)  $< --proto_path=. --cpp_out=.


%.d: %.cpp
	@rm -f $@; \
	$(cc) $(cc_flags) -I./public -I./private -MM $< > $@.$$$$ || exit 1;\
	sed -r 's,^(.*)\.o:,$@ $*\.o:,' $@.$$$$ > $@;  \
	rm -f $@.$$$$

%.pb.d: %.pb.cc
	@rm -f $@; \
	$(cc) $(cc_flags) -MM $< > $@.$$$$;  \
	sed -r 's,^(.*)\.o:,$@ $*\.o:,' $@.$$$$ > $@;  \
	rm -f $@.$$$$

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include $(pb_objects:.pb.o=.pb.d)
-include $(codedef_objects:.codedef.o=.codedef.d)
-include $(objects:.o=.d)
endif
endif

.PHONY: clean
clean:
	rm -rf *.temp
	rm -f *.o *.d *.d.*
	rm -f $(target_lib).a
	rm -f $(CONF_DIR)/protobuf.codedef.private.xml
	rm -f $(CONF_DIR)/protobuf.codedef.public.xml
	rm -f $(PB_DEFINE_DIR)/*.d
	rm -f $(PB_DEFINE_DIR)/*.h
	rm -f $(PB_DEFINE_DIR)/*.cpp
	rm -f $(PB_DEFINE_DIR)/*.cc
	rm -f $(PB_DEFINE_DIR)/*.o
	rm -f $(PB_DEFINE_DIR)/*.xml
	rm -f $(PB_DEFINE_DIR)/*.d.*

.PHONY: distclean
distclean: clean

