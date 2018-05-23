export GOOGLE_PROTOBUF_INSTALLED_DIR = $(LIBS_DIR)/google_protobuf/installed
export google_protobuf_cc_flag       = -I$(GOOGLE_PROTOBUF_INSTALLED_DIR)/include
export google_protobuf_ln_flag       = -L$(GOOGLE_PROTOBUF_INSTALLED_DIR)/lib -lprotobuf

export MYSQLCPPCONN_INSTALLED_DIR = $(LIBS_DIR)/mysql++/installed
export mysqlcppconn_cc_flag       = $(shell $(MYSQL_CONFIG_CMD) --include)\
									-I$(MYSQLCPPCONN_INSTALLED_DIR)/include
export mysqlcppconn_ln_flag       = $(shell $(MYSQL_CONFIG_CMD) --libs_r)\
									-L$(MYSQLCPPCONN_INSTALLED_DIR)/lib -lmysqlpp -Wl,-rpath=$(MYSQLCPPCONN_INSTALLED_DIR)/lib

export HIREDIS_INSTALLED_DIR	= $(LIBS_DIR)/hiredis/installed
export hiredis_cc_flag		= -I$(HIREDIS_INSTALLED_DIR)/include
export hiredis_ln_flag		= -L$(HIREDIS_INSTALLED_DIR)/lib -lhiredis

export PROTOCOL_DIR     = $(ROOT_DIR)/protocol

export base_cc_flag = -I$(ROOT_DIR) -I$(BASE_DIR) -I$(ROOT_DIR)/processes -I$(ROOT_DIR)/permanent
export base_ln_flag = \
	-L$(ROOT_DIR)/base -lbase\
	-L$(ROOT_DIR)/permanent -lpermanent\
	-L$(ROOT_DIR)/water/net -lnet\
	-L$(ROOT_DIR)/water/componet -lcomponet\
	-L$(ROOT_DIR)/water/dbadaptcher -ldbadaptcher\
	-L$(ROOT_DIR)/water/coroutine -lcoroutine\


export dependent_moudles =\
	$(ROOT_DIR)/base/libbase.a\
	$(ROOT_DIR)/water/net/libnet.a\
	$(ROOT_DIR)/water/componet/libcomponet.a\
	$(ROOT_DIR)/water/coroutine/libcoroutine.a\
	$(ROOT_DIR)/water/dbadaptcher/libdbadaptcher.a


SUBDIR =  \
		  router\
		  gateway\
		  lobby\


#		  router\
#		  http\


all: targets

targets:
	@for subdir in $(SUBDIR); do  \
		$(MAKE) -C $$subdir || exit 1 ; \
		done

.PHONY: clean
clean:
	@for subdir in $(SUBDIR); do  \
		(cd $$subdir && $(MAKE) clean); \
		done

.PHONY: distclean
distclean:
	@for subdir in $(SUBDIR); do  \
		(cd $$subdir && $(MAKE) distclean); \
		done

ctags:
	ctags -R

