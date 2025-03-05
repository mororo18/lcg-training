CC=gcc
CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -Wconversion -g

SRC = main.c
TARGET = game

RAYLIB_DIR = $(CURDIR)/external/raylib/src

ifeq ($(OS),Windows_NT)
	RAYLIB = raylib.dll

    RUN_CMD = cmd /C "set PATH=$(RAYLIB_DIR);%PATH% && .\$(TARGET).exe"
else
	RAYLIB = libraylib.so
	#LINKS = -lGL -lm -lpthread -ldl -lrt -lX12

	RUN_CMD = LD_LIBRARY_PATH=$(RAYLIB_DIR) ./$(TARGET)
endif

RAYLIB_INC = -I$(RAYLIB_DIR) -L$(RAYLIB_DIR) -lraylib

.PHONY: all clean clean-all

all: $(TARGET)

$(TARGET): $(SRC) $(RAYLIB_DIR)/$(RAYLIB)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYLIB_INC)

$(RAYLIB_DIR)/$(RAYLIB):
	$(MAKE) PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED -C $(RAYLIB_DIR) 

debug: clean
debug: CFLAGS += -DDEBUG
debug: run

run: clean $(TARGET)
	$(RUN_CMD)

clean:
	$(RM) $(TARGET) $(RAYLIB_DIR)/$(RAYLIB)

clean-all: clean
	$(MAKE) -C $(RAYLIB_DIR) clean
