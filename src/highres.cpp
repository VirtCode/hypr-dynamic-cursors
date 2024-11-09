#include "globals.hpp"
#include "plugins/PluginAPI.hpp"
#include <chrono>
#include <cmath>
#include <hyprlang.hpp>

#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp> // required so we don't "unprivate" chrono
#define private public
#include "src/managers/CursorManager.hpp"
#undef private

#include "highres.hpp"
#include "config/config.hpp"
#include "src/debug/Log.hpp"

CHighresHandler::CHighresHandler() {
    // load stuff on creation
    update();

    // and reload on config reload
    static const auto PCALLBACK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "configReloaded", [&](void* self, SCallbackInfo&, std::any data) {
        update();
    });
}

static void hcLogger(enum eHyprcursorLogLevel level, char* message) {
    if (level == HC_LOG_TRACE) return;
    Debug::log(NONE, "[hc (dynamic)] {}", message);
}

void CHighresHandler::update() {
    static auto* const* PENABLED = (Hyprlang::INT* const*) getConfig(CONFIG_HIGHRES_ENABLED);
    static auto* const* PUSEHYPRCURSOR = (Hyprlang::INT* const*) getHyprlandConfig("cursor:enable_hyprcursor");
    static auto* const* PSIZE = (Hyprlang::INT* const*) getConfig(CONFIG_HIGHRES_SIZE);

    static auto* const* PSHAKE_BASE= (Hyprlang::FLOAT* const*) getConfig(CONFIG_SHAKE_BASE);
    static auto* const* PSHAKE = (Hyprlang::INT* const*) getConfig(CONFIG_SHAKE); // currently only needed for shake

    if (!**PENABLED || !**PUSEHYPRCURSOR || !**PSHAKE) {
        // free manager if no longer enabled
        if (manager) {
            manager = nullptr;
            texture = nullptr;
            buffer = nullptr;
        }

        return;
    }

    std::string name = g_pCursorManager->m_szTheme;
    unsigned int size = **PSIZE != -1 ? **PSIZE : std::round(g_pCursorManager->m_sCurrentStyleInfo.size * **PSHAKE_BASE * 1.5f); // * 1.5f to accomodate for slight growth

    // we already have loaded the same theme and size
    if (manager && loadedName == name && loadedSize == size)
        return;

    auto options = Hyprcursor::SManagerOptions();
    options.logFn = hcLogger;
    options.allowDefaultFallback = true;

    manager = std::make_unique<Hyprcursor::CHyprcursorManager>(name.empty() ? nullptr : name.c_str(), options);
    if (!manager->valid()) {
        Debug::log(ERR, "Hyprcursor for dynamic cursors failed loading theme \"{}\", falling back to pixelated trash.", name);

        manager = nullptr;
        texture = nullptr;
        buffer = nullptr;
        return;
    }

    auto time = std::chrono::system_clock::now();

    Debug::log(INFO, "Loading hyprcursor theme {} of size {} for dynamic cursors, this might take a while!", name, size);
    style = Hyprcursor::SCursorStyleInfo { size };
    manager->loadThemeStyle(style);
    loadedSize = size;
    loadedName = name;

    float ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time).count();
    Debug::log(INFO, "Loading finished, took {}ms", ms);
}

void CHighresHandler::loadShape(const std::string& name) {
    static auto const* PFALLBACK = (Hyprlang::STRING const*) getConfig(CONFIG_HIGHRES_FALLBACK);

    if (!manager) return;

    Hyprcursor::SCursorShapeData shape = manager->getShape(name.c_str(), style);

    // try load fallback image
    if (shape.images.size() == 0) {
        shape = manager->getShape(*PFALLBACK, style);

        if (shape.images.size() == 0) {
            Debug::log(WARN, "Failed to load fallback shape {}, for shape {}!", *PFALLBACK, name);

            texture = nullptr;
            buffer = nullptr;
            return;
        }
    }

    buffer = makeShared<CCursorBuffer>(
        shape.images[0].surface,
        Vector2D{shape.images[0].size, shape.images[0].size},
        Vector2D{shape.images[0].hotspotX, shape.images[0].hotspotY}
    );

    texture = makeShared<CTexture>(buffer);
}

SP<CTexture> CHighresHandler::getTexture() {
    return texture;
}

SP<CCursorBuffer> CHighresHandler::getBuffer() {
    return buffer;
}
