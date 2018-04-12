
include ../Makefile.include

ifeq ($(OS), Darwin)
  LIBFLAGS = -dynamiclib
else
  LIBFLAGS = -shared
endif

SRC = bte.c
OBJ = $(SRC:.c=.o)

CFLAGS += `xml2-config --cflags`
LIBS += `xml2-config --libs` -lexpect -ltcl

.PHONY: all clean

all: $(BIN_TARGET)

.c.o:
	$(CC) $(CFLAGS) -g -c $< -o $@

$(BIN_TARGET): $(OBJ)
	$(CC) -g -o $@ $^ $(LIBS)

clean:
	@find . \( -name \*.o -o -name \*.a -o -name \*.so \) -exec rm {} \;
	@rm -f $(BIN_TARGET)
