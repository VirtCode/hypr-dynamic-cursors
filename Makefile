PLUGIN_NAME=dynamic-cursors

SOURCE_FILES := $(wildcard ./src/*.cpp ./src/*/*.cpp ./src/*/*/*.cpp)
HEADER_FILES := $(wildcard ./src/*.hpp ./src/*/*.hpp ./src/*/*/*.hpp)

OBJECT_FILES := $(patsubst ./src/%.cpp, out/%.o, $(SOURCE_FILES))
CXXFLAGS := -Wall -fPIC -std=c++26 -g \
	$(shell pkg-config --cflags hyprland pixman-1 libdrm | sed 's#-I\([^ ]*/hyprland\)\($$\| \)#-I\1 -I\1/src #g')

OUTPUT=out/$(PLUGIN_NAME).so

ifeq ($(CXX),g++)
    CXXFLAGS += --no-gnu-unique
endif

.PHONY: all clean load unload format

all: $(OUTPUT)

$(OUTPUT): $(OBJECT_FILES)
	$(CXX) -shared $^ $(LDFLAGS) -o $@

# Compile step (object file per .cpp)
out/%.o: ./src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(EXTRA_CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OUTPUT) $(OBJECT_FILES)

format:
	clang-format -i $(HEADER_FILES) $(SOURCE_FILES)

load: all unload
	hyprctl plugin load ${PWD}/$(OUTPUT)

unload:
	hyprctl plugin unload ${PWD}/$(OUTPUT)
