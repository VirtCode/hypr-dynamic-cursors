PLUGIN_NAME=dynamic-cursors

SOURCE_FILES := $(wildcard ./src/*.cpp ./src/*/*.cpp ./src/*/*/*.cpp)
HEADER_FILES := $(wildcard ./src/*.hpp ./src/*/*.hpp ./src/*/*/*.hpp)

OBJECT_FILES := $(patsubst ./src/%.cpp, out/%.o, $(SOURCE_FILES))
DEPEND_FILES := $(OBJECT_FILES:.o=.d)
CXXFLAGS := -Wall -fPIC -std=c++26 -g \
	$(shell pkg-config --cflags hyprland pixman-1 libdrm | sed 's#-I\([^ ]*/hyprland\)\($$\| \)#-I\1 -I\1/src #g')

OUTPUT=out/$(PLUGIN_NAME).so

# Fingerprint of the toolchain and the hyprland stack we compile against. Package managers keep the
# original mtimes of installed headers, so after a hyprland update they can be older than existing
# objects and mtime-based dependencies alone miss the change. Objects built against different
# headers would then be linked together, mixing struct layouts and crashing at runtime.
BUILD_ENV := $(CXX) $(shell $(CXX) -dumpfullversion 2>/dev/null) \
	$(shell pkg-config --modversion hyprland hyprutils aquamarine hyprlang hyprcursor hyprgraphics 2>/dev/null) \
	$(shell echo GIT_COMMIT_HASH | $(CXX) $(CXXFLAGS) -include src/version.h -E -P -x c++ - 2>/dev/null | tail -n 1)
ENV_STAMP := out/.build-env
$(shell mkdir -p out && [ "$$(cat $(ENV_STAMP) 2>/dev/null)" = "$(strip $(BUILD_ENV))" ] || echo "$(strip $(BUILD_ENV))" > $(ENV_STAMP))

ifeq ($(CXX),g++)
    CXXFLAGS += --no-gnu-unique
endif

.PHONY: all clean load unload format

all: $(OUTPUT)

$(OUTPUT): $(OBJECT_FILES)
	$(CXX) -shared $^ $(LDFLAGS) -o $@

# Compile step (object file per .cpp)
out/%.o: ./src/%.cpp $(ENV_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(EXTRA_CXXFLAGS) -MD -MP -c $< -o $@

# also rebuild objects when any header they include changes
-include $(DEPEND_FILES)

clean:
	$(RM) $(OUTPUT) $(OBJECT_FILES) $(DEPEND_FILES) $(ENV_STAMP)

format:
	clang-format -i $(HEADER_FILES) $(SOURCE_FILES)

load: all unload
	hyprctl plugin load ${PWD}/$(OUTPUT)

unload:
	hyprctl plugin unload ${PWD}/$(OUTPUT)
