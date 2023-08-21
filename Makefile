MKDIR_P = mkdir -p
BUILD_DIR = build
ARGS = -lpthread

all: runner orch

runner: builddir
	g++ src/runner.cpp -o $(BUILD_DIR)/main ${ARGS}

orch: builddir
	g++ src/orch.cpp -o $(BUILD_DIR)/orchestrator ${ARGS}

builddir:
	$(MKDIR_P) $(BUILD_DIR)


clean:
	rm -rf $(BUILD_DIR)
