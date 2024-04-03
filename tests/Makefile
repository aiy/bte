# TODO 
# valgrind --tool=memcheck --leak-check=full --num-callers=30 --show-reachable=yes --trace-children=no ~/git/bte/src/bte ~/git/bte/tests/test_stream_expect_bt.xml

include ../Makefile.include

SRC = 

TARGET = $(SRC:.c=)
OBJ = $(SRC:.c=.o)

CFLAGS := $(CFLAGS) -I ../src
#LDFLAGS += -L ../src -Bstatic -lul 
LDFLAGS += ../src/$(LIB_TARGET)

ifeq ($(OS), Darwin)
  export DYLD_LIBRARY_PATH := ../src
else
  export LD_LIBRARY_PATH := ../src
endif

.PHONY: all clean check test $(TARGET) $(OBJ)

all: $(TARGET)

$(OBJ): %.o: %.c
	@echo compiling
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJ)
	@echo building targets
	$(CC) $@.o $(LDFLAGS) -o $@

check test: $(TARGET)
	@echo testing
	sh test_bte.sh
	
clean:
	@echo cleaning
	@find . \( -name \*.o -o -name \*.a -o -name \*.so -o -name \*.zip \) -exec rm {} \;
	@rm -f $(TARGET)
