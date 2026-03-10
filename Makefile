# ============================================================
#
#  make              — build both
#  make server       — build only HTTP server
#  make console      — build only console app
#  make run-server   — build + run server
#  make clean
# ============================================================

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 -lm

SHARED = src/core/PayrollEngine.c src/utils/CSVManager.c

CONSOLE_SRCS = main.c src/ui/ConsoleUI.c $(SHARED)
SERVER_SRCS  = server_main.c src/server/HttpServer.c $(SHARED)

ifeq ($(OS),Windows_NT)
  LIBS = -lws2_32
  EXT  = .exe
else
  LIBS =
  EXT  =
  CFLAGS += -lpthread
endif

all: PayDay$(EXT) PayDayServer$(EXT)

PayDay$(EXT): $(CONSOLE_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

PayDayServer$(EXT): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

console: PayDay$(EXT)
server:  PayDayServer$(EXT)

run-server: PayDayServer$(EXT)
	./PayDayServer$(EXT)

clean:
	rm -f PayDay PayDay.exe PayDayServer PayDayServer.exe

.PHONY: all console server run-server clean
