
TOOLS_DIR = tools/

ITREE_DIR = contrib/interval_tree/

DIST_SRC = src
DIST_DEST = bin

DIST_INCLUDE = kitsune.h stackvars.h ktheaptrack.h registervars.h transform.h kitsunemisc.h ktlog.h ktthreads.h
DIST_BIN = doupd driver
DIST_LIB = libkitsune.a libkitsune-threads.a
DIST_FILES = $(DIST_BIN) $(DIST_LIB) $(DIST_INCLUDE)

DOC_DIR = doc/api/doxygen
DOC_FILE = doc/api/Doxyfile

INSTALL_PATH_LIB = /usr/local/lib/
INSTALL_PATH_BIN = /usr/local/bin/
INSTALL_PATH_INCLUDE = /usr/local/include/

DIST_DEST_FILES = $(addprefix $(DIST_DEST)/bin/, $(DIST_BIN)) \
	$(addprefix $(DIST_DEST)/lib/, $(DIST_LIB)) \
	$(addprefix $(DIST_DEST)/include/, $(DIST_INCLUDE))
all: builditree $(DIST_DEST_FILES) buildtools

.PHONY: buildintervaltree buildlib buildtools libclean clean distclean doc

builditree:
	$(MAKE) -C $(ITREE_DIR)

buildlib:
	$(MAKE) -C $(DIST_SRC)

buildtools:
	$(MAKE) -C $(TOOLS_DIR)

$(DIST_DEST)/% : buildlib
	mkdir -p $(DIST_DEST)/$(dir $*)
	cp $(DIST_SRC)/$(notdir $*) $@

libclean:
	rm -f $(DIST_DEST_FILES)

clean: libclean docclean
	$(MAKE) -C $(DIST_SRC) clean
	$(MAKE) -C $(TOOLS_DIR) clean
	$(MAKE) -C $(ITREE_DIR) clean

distclean: clean
	$(MAKE) -C $(TOOLS_DIR) distclean

doc:
	doxygen $(DOC_FILE)

docclean:
	-rm -rf $(DOC_DIR)

install: $(DIST_DEST_FILES) install-lib install-bin install-include

install-lib:
	install $(wildcard $(DIST_DEST)/lib/*) $(INSTALL_PATH_LIB)

install-bin: buildtools
	install $(wildcard $(DIST_DEST)/bin/*) $(INSTALL_PATH_BIN)

install-include:
	install -m 644 $(wildcard $(DIST_DEST)/include/*) $(INSTALL_PATH_INCLUDE)


install-clean: 
	-rm $(addprefix $(INSTALL_PATH_BIN)/, $(DIST_BIN) xfgen kttjoin ktcc) $(addprefix $(INSTALL_PATH_INCLUDE)/,$(DIST_INCLUDE)) $(addprefix $(INSTALL_PATH_LIB)/,$(DIST_LIB))
