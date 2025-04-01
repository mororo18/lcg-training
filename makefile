CFLAGS = -O3 -std=c2x -Wall -Wextra -Wpedantic -Wconversion -g -D_XOPEN_SOURCE=700 -lm

SRC_DIR = src
GAME_SRC = main.c player.c bullet.c enemy.c client.c server.c
HEADERS = bullet_array.h bullet.h client_array.h client.h config.h enemy_array.h enemy.h \
          generic_array.h packet.h player_array.h player.h server.h stacktrace.h \
          utils.h window.h

TARGET = game

OBJ_WITH_PATH = $(patsubst %.c, $(SRC_DIR)/%.o, $(GAME_SRC))
RAYLIB_DIR = $(CURDIR)/external/raylib/src

ifeq ($(OS),Windows_NT)
	RAYLIB = raylib.dll
    RUN_CMD = cmd /C "set PATH=$(RAYLIB_DIR);%PATH% && .\$(TARGET).exe"
else
	RAYLIB = libraylib.so
	RUN_CMD = LD_LIBRARY_PATH=$(RAYLIB_DIR) ./$(TARGET)
endif

RAYLIB_INC = -I$(RAYLIB_DIR) -L$(RAYLIB_DIR) -lraylib

.PHONY: all clean clean-all

$(TARGET): $(OBJ_WITH_PATH) $(RAYLIB_DIR)/$(RAYLIB)
	$(CC) -o $@ $^ $(RAYLIB_INC) $(CFLAGS) 

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/*.h)
	$(CC) -c $< -o $@ $(RAYLIB_INC) $(CFLAGS) 

$(RAYLIB_DIR)/$(RAYLIB):
	$(MAKE) PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED -C $(RAYLIB_DIR) 

debug: clean
debug: CFLAGS += -DDEBUG
debug: run

run: clean $(TARGET)
	$(RUN_CMD) --client

server-run: clean $(TARGET)
	$(RUN_CMD) --server

clean:
	$(RM) $(TARGET) $(OBJ_WITH_PATH) $(RAYLIB_DIR)/$(RAYLIB)

clean-all: clean
	$(MAKE) -C $(RAYLIB_DIR) clean
