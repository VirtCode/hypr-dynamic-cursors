PLUGIN_NAME=dynamic-cursors
SOURCE_FILES=$(wildcard ./src/*.cpp ./src/*/*.cpp)

all: $(PLUGIN_NAME).so

$(PLUGIN_NAME).so: $(SOURCE_FILES)
	mkdir -p out
	g++ -shared -Wall --no-gnu-unique -fPIC $(SOURCE_FILES) -g -I /usr/include/hyprland/src `pkg-config --cflags pixman-1 libdrm hyprland` -std=c++23 -o out/$(PLUGIN_NAME).so

clean:
	rm -f out/$(PLUGIN_NAME).so
	rm -f compile_commands.json

load: all unload
	hyprctl plugin load ${PWD}/out/$(PLUGIN_NAME).so

unload:
	hyprctl plugin unload ${PWD}/out/$(PLUGIN_NAME).so
