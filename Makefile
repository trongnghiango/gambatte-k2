CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-function
INCLUDES ?= -I.

PKG_PACKAGES := gtk+-2.0 glib-2.0 gobject-2.0 gio-2.0 gmodule-2.0 xcb xcb-shm
PKG_CFLAGS   := $(shell pkg-config --cflags $(PKG_PACKAGES) 2>/dev/null)
PKG_LIBS     := $(shell pkg-config --libs $(PKG_PACKAGES) 2>/dev/null) -lm -lpthread

BIN_DIR := bin
TARGET := $(BIN_DIR)/gambatte-k2

SRCS := main.c retrocore.c gamejoypad.c file_picker.c fbink.c gray_shm.c kindle_compat.c
OBJS := $(patsubst %.c,$(BIN_DIR)/%.o,$(SRCS))

.PHONY: all directories clean package-kual

all: directories $(TARGET)

directories:
	@mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(PKG_LIBS) -o $(TARGET)
	@echo "Build successful: $(TARGET)"

$(BIN_DIR)/%.o: %.c | directories
	$(CC) $(CFLAGS) $(INCLUDES) $(PKG_CFLAGS) -c $< -o $@

package-kual: all
	@chmod +x scripts/package-kual.sh
	@./scripts/package-kual.sh

clean:
	rm -rf $(BIN_DIR) dist *.zip
