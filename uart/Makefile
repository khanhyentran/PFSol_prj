CC_FLAGS = $(COMPILER_FLAGS)
ECC_FLAGS = $(EXTRA_COMPILER_FLAGS)

MODULE_INC = $(addprefix -I, $(shell find $(RCAR_BSP_SOURCE) -type d -name 'inc*'))
INCLUDES = $(MODULE_INC) $(FREERTOS_INC)

OBJECTS = $(addsuffix .o, $(basename $(wildcard */*.c */*.S)))
#OBJECTS += $(addsuffix .o, $(basename $(wildcard */*.c) $(wildcard */*.S))

%.o: %.S
	$(COMPILER) $(ASFLAGS) -I. -c -o $@ $<

%.o: %.c
	$(COMPILER) $(CC_FLAGS) $(ECC_FLAGS) $(INCLUDES) -o $@ $<

libs: $(OBJECTS)
	@echo "Compiling libs"
	$(ARCHIVER) -r $(BSPLIBS) $^

distclean: clean
clean:
	rm -rf $(OBJECTS)

.PHONY: all clean distclean

