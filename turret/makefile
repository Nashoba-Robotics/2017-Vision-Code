BUILD := build

CAMERA ?= 0 

CXXFLAGS := $(shell pkg-config opencv --cflags --libs)
CXXFLAGS += -O3
ERR_FLAGS := -Wall -Werror -Wextra

.PHONY: camera run clean

camera: $(BUILD)/camera

$(BUILD)/camera: camera.cpp tcp_client.cpp
	-@mkdir build
	g++ $(CXXFLAGS) -o $(BUILD)/camera $^ $(ERR_FLAGS)

run: $(BUILD)/camera
	./$< $(CAMERA)

clean:
	rm -rf $(BUILD)
