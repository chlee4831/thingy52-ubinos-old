LIBRARY_DIR ?= ../library

help: common-help

%: common-% ;

-include $(LIBRARY_DIR)/ubinos/make/custom.mk
include $(LIBRARY_DIR)/ubinos/make/common.mk

