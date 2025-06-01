# See LICENSE for license details.

CXX_FLAGS += -std=c++20 -O3 -Wall
PAR_FLAG = -fopenmp

ifneq (,$(findstring icpc,$(CXX)))
	PAR_FLAG = -openmp
endif

ifneq ($(SERIAL), 1)
	CXX_FLAGS += $(PAR_FLAG)
endif

ifeq ($(USE_128), 1)
	CXX_FLAGS += -DUSE_128
endif

KERNELS = pivotscale pivotscale-sweep
SUITE = $(KERNELS) converter

.PHONY: all
all: $(SUITE)

% : src/%.cc src/*.h
	$(CXX) $(CXX_FLAGS) $< -o $@ $(LD_FLAGS)


.PHONY: clean
clean:
	rm -f $(SUITE)
