BINARY := power
BINDIR := build
SRCDIR := src

HDRS := $(wildcard $(SRCDIR)/*.h)
SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(BINDIR)/%.o)
BIN_FILE := $(BINDIR)/$(BINARY)

CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=gnu99
LDFLAGS := -lrt -lm
LD := $(CC)

.PHONY: all
all : $(BIN_FILE)
	sudo setcap cap_sys_rawio=ep $(BIN_FILE)
	 
$(BIN_FILE) : $(OBJS)
	@echo linking $@
	$(maketargetdir)
	$(LINK.c) $(OUTPUT_OPTION) $^

$(BINDIR)/%.o : $(SRCDIR)/%.c
	@echo compiling $<
	$(maketargetdir)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

define maketargetdir
	-@mkdir -p $(dir $@) > /dev/null 2>&1
endef

.PHONY: clean
clean :
	rm -f $(BIN_FILE) $(OBJS)
	rm -rf $(BINDIR)
