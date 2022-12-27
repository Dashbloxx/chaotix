CFLAGS += \
	-isystemlib \
	-isystem.. \
	-isystem../common \
	-isystem../kernel/api
LDFLAGS := -L.

BIN_TARGET_NAMES := \
	cal \
	cat \
	clear \
	cp \
	date \
	echo \
	env \
	fib \
	gui \
	halt \
	imgview \
	init \
	getfb \
	init-test \
	kill \
	ln \
	ls \
	mandelbrot \
	mkdir \
	mouse-cursor \
	mv \
	poweroff \
	ps \
	pwd \
	reboot \
	rm \
	rmdir \
	run-tests \
	sh \
	sleep \
	touch \
	wc \
	xv6-usertests
OUTDIR := ../base/bin

BIN_TARGETS := $(BIN_TARGET_NAMES:%=$(OUTDIR)/%)
BIN_TARGET_OBJS := $(BIN_TARGET_NAMES:=.o)
BIN_TARGET_DEPS := $(BIN_TARGET_NAMES:=.d)

LIB_TARGET := libc.a

LIB_OBJS := \
	../common/math.o \
	../common/string.o \
	../common/strings.o \
 	../common/stdio.o \
 	../common/stdlib.o \
	lib/crt0.o \
	lib/dirent.o \
	lib/errno.o \
	lib/panic.o \
	lib/stdio.o \
	lib/stdlib.o \
	lib/string.o \
	lib/syscall.o \
	lib/time.o \
	lib/unistd.o

LIB_DEPS := $(LIB_OBJS:.o=.d)

.PHONY: all clean

all: $(BIN_TARGETS)

$(BIN_TARGETS): $(OUTDIR)/% : %.o $(LIB_TARGET)
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lc

$(LIB_TARGET): $(LIB_OBJS)
	@echo "[AR] $@"
	@$(AR) rcs $@ $^

.c.o:
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

clean:
	$(RM) $(BIN_TARGETS) $(BIN_TARGET_OBJS) $(BIN_TARGET_DEPS) $(LIB_TARGET) $(LIB_OBJS) $(LIB_DEPS)

-include $(BIN_TARGET_DEPS)
-include $(LIB_DEPS)
