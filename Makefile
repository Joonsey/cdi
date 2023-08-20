MKDIR_P = mkdir -p
BUILD_DIR = build

all: runner orch

runner: builddir
	g++ src/runner.cpp -o $(BUILD_DIR)/main

orch: builddir
	g++ src/orch.cpp -o $(BUILD_DIR)/orchestrator

builddir:
	$(MKDIR_P) $(BUILD_DIR)


clean:
	rm -rf $(BUILD_DIR)
