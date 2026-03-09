PLUGIN_NAME=dynamic-cursors

SOURCE_FILES := $(wildcard ./src/*.cpp ./src/*/*.cpp)
OBJECT_FILES := $(patsubst ./src/%.cpp, out/%.o, $(SOURCE_FILES))
CXX_FLAGS := -Wall -fPIC -std=c++26 -g \
	$(shell pkg-config --cflags hyprland pixman-1 libdrm | sed 's#-I\([^ ]*/hyprland\)\($$\| \)#-I\1 -I\1/src #g')

OUTPUT=out/$(PLUGIN_NAME).so

ifeq ($(CXX),g++)
    EXTRA_FLAGS = --no-gnu-unique
else
    EXTRA_FLAGS =
endif

.PHONY: all clean load unload

all: $(OUTPUT)

$(OUTPUT): $(OBJECT_FILES)
	$(CXX) -shared $^ -o $@

# Compile step (object file per .cpp)
out/%.o: ./src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $(LDFLAGS) $(EXTRA_FLAGS) -c $< -o $@

clean:
	$(RM) $(OUTPUT) $(OBJECT_FILES)

load: all unload
	hyprctl plugin load ${PWD}/$(OUTPUT)

unload:
	hyprctl plugin unload ${PWD}/$(OUTPUT)
