.POSIX:
.SUFFIXES:

all: demo main scratch_test

fresh:
	$(MAKE) superclean
	$(MAKE) all

CONFIG ?= debug
BIN := bin
OBJ := obj
SRC := src
LIB := libs
SHD := shaders
OUT := $(BIN)/$(CONFIG)
TMP := $(OBJ)/$(CONFIG)

CC       := clang
LDFLAGS  :=
# LDLIBS   := -lm
CPPFLAGS := -I$(SRC) -I$(LIB) -I$(LIB)/sokol -I$(SHD)
CFLAGS   := -std=c11
CFLAGS   := -MMD
CFLAGS   += -Werror -Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum
# CFLAGS   += -Weverything
# CFLAGS   += -Wno-declaration-after-statement -Wno-reserved-identifier -Wno-padded -Wno-anon-enum-enum-conversion -Wno-pre-c11-compat -Wno-poison-system-directories -Wno-bad-function-cast -Wno-implicit-float-conversion -Wno-double-promotion -Wno-used-but-marked-unused -Wno-nullable-to-nonnull-conversion -Wno-switch-enum -Wno-cast-qual -Wno-four-char-constants -Wno-missing-prototypes -Wno-cstring-format-directive -Wno-objc-messaging-id -Wno-selector
CFLAGS   += -Wno-empty-translation-unit -Wno-variadic-macro-arguments-omitted
ifeq ($(CONFIG), debug)
CFLAGS   += -g3 -O0 -DDEBUG
CFLAGS   += -fsanitize=address -fsanitize=undefined -fstack-protector-strong
LDFLAGS  += -fsanitize=address
else
CFLAGS   += -g0 -O3 -DNDEBUG
endif

SOURCES := $(shell find $(SRC) -name "*.c" ! -name "*_entry.c" ! -name "*_test.c")
OBJECTS := $(patsubst $(SRC)/%.c,$(TMP)/%.o,$(SOURCES))

SHD_SOURCES := $(wildcard $(SHD)/*.glsl)
SHD_TARGETS := $(patsubst %,%.h,$(SHD_SOURCES))
SHD_MK      := $(SHD)/gen.mk

FRAMEWORKS := AudioToolbox Cocoa Metal QuartzCore
LDFLAGS += $(foreach framework,$(FRAMEWORKS),-framework $(framework))

$(OBJECTS): $(TMP)/%.o : $(SRC)/%.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

demo: $(OUT)/demo
DEMO_OBJECTS := $(OBJECTS) $(TMP)/sokol/sokol.o $(TMP)/demo_entry.o

$(OUT)/demo: $(DEMO_OBJECTS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(DEMO_OBJECTS) -o $@

$(TMP)/demo_entry.o: $(SRC)/demo_entry.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

main: $(OUT)/main
MAIN_OBJECTS := $(OBJECTS) $(TMP)/sokol/sokol.o $(TMP)/main_entry.o

$(OUT)/main: $(MAIN_OBJECTS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(MAIN_OBJECTS) -o $@

$(TMP)/main_entry.o: $(SRC)/main_entry.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

SOKOL_SRC := $(LIB)/sokol/sokol.m $(LIB)/sokol/sokol_gfx.c
SOKOL_OBJ := $(TMP)/sokol/sokol.o

$(SOKOL_OBJ): $(SOKOL_SRC)
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(filter-out -Wswitch-enum, $(CFLAGS)) -fobjc-arc $< -o $@

# TESTS
CPPFLAGS += -I$(LIB)/unity
UNITY_SRC := $(LIB)/unity/unity.c
UNITY_OBJ := $(TMP)/unity/unity.o

$(UNITY_OBJ): $(UNITY_SRC)
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

SOKOL_OBJ_NO_ENTRY := $(TMP)/sokol/sokol_no_entry.o

$(SOKOL_OBJ_NO_ENTRY): $(SOKOL_SRC)
	$(CC) -c $(CPPFLAGS) $(filter-out -Wswitch-enum, $(CFLAGS)) -fobjc-arc -DSOKOL_NO_ENTRY $< -o $@

scratch_test: $(OUT)/scratch_test
SCRATCH_TEST_OBJECTS := $(OBJECTS) $(UNITY_OBJ) $(TMP)/sokol/sokol_no_entry.o $(TMP)/scratch_test.o

$(OUT)/scratch_test: $(SCRATCH_TEST_OBJECTS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(SCRATCH_TEST_OBJECTS) -o $@

$(TMP)/scratch_test.o: $(SRC)/scratch_test.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BIN)
	rm -rf $(OBJ)

superclean: clean
	rm -f  $(SHD)/*.glsl.h

# Force "makefile remaking" of any shader sources that were altered. Going this route vs an explict
# prerequisite for the $(OBJECTS) target means that altering a shader file does not cascade into a
# recompilation of all $(SOURCES). It also has lower overhead than calling $(MAKE) $(SHD_TARGETS)
# for every %.o in $(OBJECTS).
-include $(SHD_MK)

SHDC := tools/sokol-shdc

$(SHD_MK): $(SHD_TARGETS)

$(SHD_TARGETS): %.h : %
	$(SHDC) -i $< -o $@ -l metal_macos

-include $(patsubst $(SRC)/%.c,$(TMP)/%.d,$(SOURCES))
