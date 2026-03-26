.POSIX:
.SUFFIXES:

all: exe

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
EXE := $(OUT)/demo

CC       := clang
LDFLAGS  :=
# LDLIBS   := -lm
CPPFLAGS := -I$(SRC) -I$(LIB)/sokol -I$(SHD)
CFLAGS   := -std=c11
CFLAGS   := -MMD
CFLAGS   += -Werror -Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum
ifeq ($(CONFIG), debug)
CFLAGS   += -g3 -O0 -DDEBUG
CFLAGS   += -fsanitize=address -fsanitize=undefined -fstack-protector-strong
LDFLAGS  += -fsanitize=address
else
CFLAGS   += -g0 -O3 -DNDEBUG
endif

SOURCES := $(shell find $(SRC) -name "*.c")
OBJECTS := $(patsubst $(SRC)/%.c,$(TMP)/%.o,$(SOURCES))

SHD_SOURCES := $(wildcard $(SHD)/*.glsl)
SHD_TARGETS := $(patsubst %,%.h,$(SHD_SOURCES))
SHD_MK      := $(SHD)/gen.mk

exe: $(EXE)

ALL_OBJECTS := $(OBJECTS) $(TMP)/sokol.o
FRAMEWORKS := AudioToolbox Cocoa Metal QuartzCore

$(EXE): $(ALL_OBJECTS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(foreach framework,$(FRAMEWORKS),-framework $(framework)) $(ALL_OBJECTS) -o $@

$(OBJECTS): $(TMP)/%.o : $(SRC)/%.c
	mkdir -p $(@D)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(TMP)/sokol.o: $(SRC)/sokol.m
	$(CC) -c $(CPPFLAGS) $(filter-out -Wswitch-enum, $(CFLAGS)) -fobjc-arc $< -o $@

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
-include $(TMP)/sokol.d
