objects = \
    \
    listener.o\
    connection.o\
    socket.o\
    endpoint.o\
    epoller.o\
    packet.o\
	tcp_packet.o\
	http_packet.o\
	buffered_connection.o\


clang_objects= \
	http-parser/http_parser.o

CC = g++
cc = g++

cc_flags = $(CPP_STD_VER) -ggdb -Wall -Werror \
		   $(optimization_flag)\
		   -I$(BASE_DIR)


target = libnet

.PHONY:
all: $(target).a


$(target).a : $(objects) $(clang_objects)
	ar rs $(target).a $(objects) $(clang_objects)

$(objects): %.o: %.cpp
	$(cc) $(cc_flags) -c $< -o $@

$(clang_objects): %.o: %.c
	$(cc) $(cc_flags) -c $< -o $@ 


%.d: %.cpp
	@#set -x ; 
	@set -e ; \
	rm -f $@; \
	$(cc) -MM $(cc_flags) $< > $@.$$$$;  \
	sed -r 's,^(.*)\.o:,$@ $*\.o:,' $@.$$$$ > $@;  \
	rm -f $@.$$$$

ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), distclean)
-include $(objects:.o=.d)
endif
endif

#sed 's,/($*/)/.o[ :]*,/1.o $@ : ,g' < $@.$$$$ > $@; \

.PHONY: clean distclean
clean:
	rm -f $(objects) $(clang_objects) *.d *.d.* */*.d
	rm -f $(target).a

distclean: clean
