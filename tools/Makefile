
COMP_DIR = cil-1.3.7
TOOL_DIR = ocaml-src/tools

SRC_SCRIPT = scripts/ktcc.sh
DST_SCRIPT = ../bin/bin/ktcc

.PHONY: buildcomp buildtools clean distclean

all: buildtools

$(COMP_DIR)/Makefile: $(COMP_DIR)/Makefile.in $(COMP_DIR)/configure
	cd $(COMP_DIR) && ./configure

buildcomp: $(COMP_DIR)/Makefile 
	$(MAKE) -j1 -C $(COMP_DIR)
	sed "s:CIL_BIN_DIR:$(shell pwd)/$(COMP_DIR):" $(SRC_SCRIPT) > $(DST_SCRIPT)
	chmod +x $(DST_SCRIPT)

buildtools: buildcomp
	$(MAKE) -j1 -C $(TOOL_DIR)

clean:
	$(MAKE) -C $(COMP_DIR) clean
	$(MAKE) -C $(TOOL_DIR) clean
	rm -f $(DST_SCRIPT)

distclean: clean
	$(MAKE) -C $(COMP_DIR) distclean
