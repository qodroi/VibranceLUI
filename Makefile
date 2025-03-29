# SPDX-License-Identifier: GPL-3.0-only

CC 		= gcc
CFLAGS 	= -Iinclude -lXext -lXinerama -lX11 -lXNVCtrl \
			 `pkg-config --cflags gtk4` `pkg-config --libs gtk4 gmodule-2.0`
CFLAGS 	+= -Wall -fno-strict-aliasing -fno-omit-frame-pointer -Wformat=2
CFLAGS	+= -ggdb -O2 # -DDEBUG=1 #-fsanitize=address
TARGET 	= vibrancelui
SOURCE	= vibrancelui.c vgui.c vhook.c ghashtable.c

$(TARGET): $(SOURCE)
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm $(TARGET)

run: $(TARGET)
	./$(TARGET)
