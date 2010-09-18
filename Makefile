SUBDIRS=src

.PHONY: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@


SRC_TARGETS=clean install

$(SRC_TARGETS):
	$(MAKE) -C src $@
