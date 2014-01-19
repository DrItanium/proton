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
	@electron -f2 cmd/generate-startup-script.clp > src/proton
	@chmod +x src/proton
	@rm /tmp/generate-startup-script.input
	@cp cmd/entry-template.clp src/entry.clp
	@echo "(defmountpoint proton \"$(PROTON_ROOT)\")" >> src/entry.clp

install: 
	@echo Installing filesystem components
	@mkdir -p $(PREFIX)/bin
	@cp src/proton $(PREFIX)/bin
	@mkdir -p $(PROTON_ROOT)
	@cp -r src/lib/ $(PROTON_ROOT)
	@cp src/entry.clp $(PROTON_ROOT)

uninstall:
	@echo Uninstalling filesystem components
	@rm $(PREFIX)/bin/proton 
	@rm -rf $(PROTON_ROOT)

clean:
	@echo Cleaning
	@rm -f src/entry.clp src/proton
