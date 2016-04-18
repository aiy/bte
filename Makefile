
SUBDIRS = src tests

.PHONY: all clean test subdirs $(SUBDIRS)

all: subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS): 
	@echo building $@
	@make -C $@

tests: src

test: tests
	@echo testing
	@make -C tests test
clean:
	for t in $(SUBDIRS); do echo cleaning $$t; make -C $$t clean; done ; true

