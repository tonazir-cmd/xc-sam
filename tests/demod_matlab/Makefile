# =============================================================================
# Makefile — OFDMMod Test
# =============================================================================

CXX := g++

BUILD ?= debug

ifeq ($(BUILD),release)
    OPT_FLAGS := -O3 -DNDEBUG
else
    OPT_FLAGS := -O0 -g
endif

# -----------------------------------------------------------------------------
# Path to built lib — adjust if needed
# -----------------------------------------------------------------------------

ITPP_LDFLAGS := $(shell pkg-config --libs   itpp)
ITPP_RAW_FLAGS := $(shell pkg-config --cflags itpp)

ifeq ($(BUILD),release)
    ITPP_CFLAGS := $(ITPP_RAW_FLAGS)
else
    ITPP_CFLAGS := $(filter-out -DNDEBUG -O3 -O2, $(ITPP_RAW_FLAGS))
endif
# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
LIB_ROOT := ../../lib
LIB_SAM_LIB := $(LIB_ROOT)/bin/sam
LIB_SAM_INC := $(LIB_ROOT)/include
LOCAL_INC := include
TEST_DIR  := test
SRC_DIR   := src
BIN_DIR   := bin

# -----------------------------------------------------------------------------
# Flags
# -----------------------------------------------------------------------------
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic \
            $(OPT_FLAGS)                          \
            -I$(LIB_SAM_INC)                      \
            -I$(LOCAL_INC)                        \
            $(ITPP_CFLAGS)

LDFLAGS := -L$(LIB_SAM_LIB) -lsam $(ITPP_LDFLAGS)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------
SRCS := $(wildcard $(SRC_DIR)/*.cpp) $(TEST_DIR)/test.cpp
TEST_BIN := $(BIN_DIR)/test.exe

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------
.PHONY: all clean

all: $(TEST_BIN)

$(TEST_BIN): $(SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	@echo "[LD]  $@"

clean:
	rm -rf $(BIN_DIR)/
	@echo "[CLEAN] done"

run:
	./$(TEST_BIN)