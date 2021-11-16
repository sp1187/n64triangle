BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = triangle.c

all: triangle.z64

$(BUILD_DIR)/triangle.elf: $(src:%.c=$(BUILD_DIR)/%.o)

triangle.z64: N64_ROM_TITLE="Tricky Triangles"

clean:
	rm -rf $(BUILD_DIR) triangle.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
