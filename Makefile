SRC := src/
SOURCES:=$(filter %.c,$(SRC))
default:
	@echo
	@$(SOURCES)
	@echo

	
