#include <hyprland/src/managers/CursorManager.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/helpers/memory/Memory.hpp>
#include <hyprcursor/hyprcursor.hpp>

#include <hyprutils/math/Vector2D.hpp>
#include <string>

class CHighresHandler {
public:
    CHighresHandler();

    /* refreshes the hyprcursor theme and stuff, should be called if config values change */
    void update();

    /* update the currently loaded shape */
    void loadShape(const std::string& name);

    SP<CTexture> getTexture();
    SP<CCursorBuffer> getBuffer();

private:
    bool enabled = true;

    Hyprcursor::SCursorStyleInfo style;
    UP<Hyprcursor::CHyprcursorManager> manager;

    /* keep track of loaded theme so we don't reload unnessecarily (<- i'm almost certain there's a typo in this word) */
    unsigned int loadedSize = -1;
    std::string loadedName = "";

    /* current texture and hotspot */
    std::string shape = "";
    SP<CTexture> texture;
    SP<CCursorBuffer> buffer;
};
