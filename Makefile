include config.mk
.PHONY: all clean options

all: options fs

options:
	@echo options for proton
	@echo Prefix is $(PREFIX)
	@echo Proton Root is $(PROTON_ROOT)

fs:
	@echo Generating filesystem
	@echo "(deffacts donuts (root electron \"$(PROTON_ROOT)\"))" > /tmp/generate-startup-script.input
	@electron -f2 cmd/generate-startup-script.clp >> proton
	@chmod +x proton
	@echo "(deffacts donuts (root neutron \"$(PROTON_ROOT)\"))" > /tmp/generate-startup-script.input
	@electron -f2 cmd/generate-startup-script.clp >> quark
	@chmod +x quark
	@rm /tmp/generate-startup-script.input
	@cp cmd/entry-template.clp entry.clp
	@echo "(batch* \"$(PROTON_ROOT)/lib/mount.clp\")" >> entry.clp
	@echo "(defmountpoint proton \"$(PROTON_ROOT)\")" >> entry.clp

install:
	@echo Installing filesystem components
	@mkdir -p $(PREFIX)/bin
	@cp proton quark $(PREFIX)/bin
	@mkdir -p $(PROTON_ROOT)
	@cp -r lib/ $(PROTON_ROOT)
	@cp entry.clp $(PROTON_ROOT)

uninstall:
	@echo Uninstalling filesystem components
	@rm $(PREFIX)/bin/proton $(PREFIX)/bin/quark
	@rm -rf $(PROTON_ROOT)

clean:
	@echo Cleaning
	@rm -f entry.clp quark proton
