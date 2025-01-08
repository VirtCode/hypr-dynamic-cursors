PLUGIN_NAME=dynamic-cursors

SOURCE_FILES=$(wildcard ./src/*.cpp ./src/*/*.cpp)
OUTPUT=out/$(PLUGIN_NAME).so

.PHONY: all clean load unload

all: $(OUTPUT)

$(OUTPUT): $(SOURCE_FILES)
	mkdir -p out
	$(CXX) -shared -Wall --no-gnu-unique -fPIC $(SOURCE_FILES) -g `pkg-config --cflags hyprland | awk '{print $$NF "/src";}'` `pkg-config --cflags pixman-1 libdrm hyprland` -std=c++26 -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)

load: all unload
	hyprctl plugin load ${PWD}/$(OUTPUT)

unload:
	hyprctl plugin unload ${PWD}/$(OUTPUT)
