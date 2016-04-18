
include ../Makefile.include

ifeq ($(OS), Darwin)
  LIBFLAGS = -dynamiclib
else
  LIBFLAGS = -shared
endif

SRC = bte.c
OBJ = $(SRC:.c=.o)

CFLAGS=`xml2-config --cflags`
LIBS=`xml2-config --libs`

.PHONY: all clean

all: $(BIN_TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

clean:
	@find . \( -name \*.o -o -name \*.a -o -name \*.so \) -exec rm {} \;
	@rm -f $(BIN_TARGET)
