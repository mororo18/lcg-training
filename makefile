CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -Wconversion -g -rdynamic

SRC_DIR = src
GAME_SRC = player.c bullet.c enemy.c client.c
HEADERS = window.h player.h bullet.h enemy.h client.h

CLIENT_SRC = main.c $(GAME_SRC)
CLIENT = game

SERVER_SRC = server.c $(GAME_SRC)
SERVER = server

CLIENT_OBJ_WITH_PATH = $(patsubst %.c, $(SRC_DIR)/%.o, $(CLIENT_SRC))
SERVER_OBJ_WITH_PATH = $(patsubst %.c, $(SRC_DIR)/%.o, $(SERVER_SRC))

RAYLIB_DIR = $(CURDIR)/external/raylib/src

ifeq ($(OS),Windows_NT)
	RAYLIB = raylib.dll

    RUN_CMD = cmd /C "set PATH=$(RAYLIB_DIR);%PATH% && .\$(CLIENT).exe"
    RUN_SERVER_CMD = cmd /C "set PATH=$(RAYLIB_DIR);%PATH% && .\$(SERVER).exe"
else
	RAYLIB = libraylib.so
	#LINKS = -lGL -lm -lpthread -ldl -lrt -lX12

	RUN_CMD = LD_LIBRARY_PATH=$(RAYLIB_DIR) ./$(CLIENT)
	RUN_SERVER_CMD = LD_LIBRARY_PATH=$(RAYLIB_DIR) ./$(SERVER)
endif

RAYLIB_INC = -I$(RAYLIB_DIR) -L$(RAYLIB_DIR) -lraylib

.PHONY: all clean clean-all

all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_OBJ_WITH_PATH) $(RAYLIB_DIR)/$(RAYLIB)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYLIB_INC)

$(SERVER): $(SERVER_OBJ_WITH_PATH) $(RAYLIB_DIR)/$(RAYLIB)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYLIB_INC)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/%.h)
	$(CC) $(CFLAGS) -c $^ -o $@ $(RAYLIB_INC)

$(RAYLIB_DIR)/$(RAYLIB):
	$(MAKE) PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED -C $(RAYLIB_DIR) 

debug: clean
debug: CFLAGS += -DDEBUG
debug: run

run: clean $(CLIENT)
	$(RUN_CMD)

server-run: clean $(SERVER)
	$(RUN_SERVER_CMD)

clean:
	$(RM) $(CLIENT) $(CLIENT_OBJ_WITH_PATH) $(SERVER) $(SERVER_OBJ_WITH_PATH) $(RAYLIB_DIR)/$(RAYLIB)

clean-all: clean
	$(MAKE) -C $(RAYLIB_DIR) clean
