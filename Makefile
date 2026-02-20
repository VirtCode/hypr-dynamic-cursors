PLUGIN_NAME=dynamic-cursors

SOURCE_FILES := $(wildcard ./src/*.cpp ./src/*/*.cpp)
OBJECT_FILES := $(patsubst ./src/%.cpp, out/%.o, $(SOURCE_FILES))
CXX_FLAGS := -Wall --no-gnu-unique -fPIC -std=c++26 -g \
	$(shell pkg-config --cflags hyprland pixman-1 libdrm | sed 's#-I\([^ ]*/hyprland\)\($$\| \)#-I\1 -I\1/src #g')

OUTPUT=out/$(PLUGIN_NAME).so

.PHONY: all clean load unload inject uninject

all: $(OUTPUT)

$(OUTPUT): $(OBJECT_FILES)
	$(CXX) -shared $^ -o $@

# Compile step (object file per .cpp)
out/%.o: ./src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	$(RM) $(OUTPUT) $(OBJECT_FILES)

# load into current session
current: all unload
	hyprctl plugin load ${PWD}/$(OUTPUT)

uncurrent:
	hyprctl plugin unload ${PWD}/$(OUTPUT)

# load into nested session
INSTANCE=1
inject: all uninject
	hyprctl -i $(INSTANCE) plugin load ${PWD}/$(OUTPUT)

uninject:
	hyprctl -i $(INSTANCE) plugin unload ${PWD}/$(OUTPUT)
