# Dynamic Library
# >>> gcc main.c -o main.exe ./Desktop/libdemo.dll
# >>> gcc main.c -o main.exe -L./Desktop -ldemo

# Static Library
# >>> gcc main.c -o main.exe ./Desktop/libdemo.a

# Merge
# >>> gcc main.c -o main.exe -static -L./Desktop -ldemo
# >>> gcc main.c -o main.exe -static ./Desktop/libdemo.a

# Include
# >>> gcc main.c -o main.out -O2 -flto -I/usr/local/include -L/usr/local/lib -ldemo
# >>> gcc main.c -o main.out -O2 -flto -I/usr/local/include /usr/local/lib/libdemo.a

# Inspect Dynamic Library
# (macOS) >>> otool -L main.out
# (Linux) >>> ldd main.out

.PHONY: clean shared static install uninstall asan

CC = gcc
TARGET = demo

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
SRC = $(call rwildcard, ./, %.c)
CHEAD = $(call rwildcard, ./, %.h)

ASAN_FLAGS = -g -O0 -fsanitize=address -fno-omit-frame-pointer

ifeq ($(OS), Windows_NT)
	REMOVE = cmd /c del
	SHARED := lib$(TARGET).dll
	STATIC := lib$(TARGET).a
	TARGET := $(TARGET).exe
	SRC := $(subst /,\,$(SRC))
	PARAMS = -s -O3 -flto -pipe
	LIBRARY = -lws2_32 -lm
else
	REMOVE = rm -f
	SHARED := lib$(TARGET).so
	STATIC := lib$(TARGET).a
	TARGET := $(TARGET)
	SRC := $(subst \,/,$(SRC))
	PARAMS = -O3 -flto -pipe
	LIBRARY = -pthread -lm
	PREFIX ?= /usr/local
	ifeq ($(shell uname), Linux)
		LDCONFIG := ldconfig $(PREFIX)/lib
	else
		LDCONFIG := @true
	endif
endif

ifeq ($(ASAN), 1)
	PARAMS = $(ASAN_FLAGS)
endif

compilation = $(SRC:.c=.o)

$(TARGET): $(compilation)
	$(CC) -Wall -Wextra -Werror $(compilation) -o $(TARGET) $(PARAMS) $(LIBRARY)

$(compilation): %.o: %.c
	$(CC) -Wall -Wextra -Werror -c $< -o $@ $(PARAMS) -fPIC

shared: $(SHARED)
$(SHARED): $(compilation)
ifeq ($(OS), Windows_NT)
	$(CC) -Wall -Wextra -Werror -shared $(compilation) -o $(SHARED) $(PARAMS) $(LIBRARY)
else
    ifeq ($(shell uname), Darwin)
	$(CC) -Wall -Wextra -Werror -shared $(compilation) -o $(SHARED) $(PARAMS) $(LIBRARY) -Wl,-install_name,$(PREFIX)/lib/$(SHARED)
    else
	$(CC) -Wall -Wextra -Werror -shared $(compilation) -o $(SHARED) $(PARAMS) $(LIBRARY)
    endif
endif

static: $(STATIC)
$(STATIC): $(compilation)
ifeq ($(OS), Windows_NT)
	gcc-ar rcs $(STATIC) $(compilation)
else
    ifeq ($(shell uname), Darwin)
	ar rcs $(STATIC) $(compilation)
    else
	gcc-ar rcs $(STATIC) $(compilation)
    endif
endif

install: shared static
ifeq ($(OS), Windows_NT)
	@echo "Error: 'make install' is not supported on Windows."
else
	mkdir -p $(PREFIX)/bin $(PREFIX)/lib $(PREFIX)/include
	cp $(SHARED) $(STATIC) $(PREFIX)/lib/
	cp $(CHEAD) $(PREFIX)/include/
	$(LDCONFIG)
endif

uninstall:
ifeq ($(OS), Windows_NT)
	@echo "Error: 'make uninstall' is not supported on Windows."
else
	rm -f $(PREFIX)/lib/$(SHARED) $(PREFIX)/lib/$(STATIC)
	rm -f $(addprefix $(PREFIX)/include/, $(notdir $(CHEAD)))
	@if [ "$(shell uname)" = "Linux" ]; then ldconfig; echo "---- updating shared library cache ----"; fi
endif

clean:
	-$(REMOVE) $(compilation) $(TARGET) $(SHARED) $(STATIC)

asan:
	@echo "======== Rebuilding with AddressSanitizer ========"
	$(MAKE) clean
	$(MAKE) ASAN=1