#include "globals.hpp"
#include <chrono>
#include <cmath>
#include <future>
#include <hyprcursor/hyprcursor.hpp>
#include <hyprlang.hpp>

#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp> // required so we don't "unprivate" chrono
#include <hyprutils/memory/UniquePtr.hpp>
#define private public
#include <hyprland/src/managers/CursorManager.hpp>
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

    if (!isEnabled() || !**PENABLED || !**PUSEHYPRCURSOR || !**PSHAKE) {
        // free manager if no longer enabled
        if (manager) {
            manager = nullptr;
            texture = nullptr;
            buffer = nullptr;
        }

        return;
    }

    std::string name = g_pCursorManager->m_theme;
    unsigned int size = **PSIZE != -1 ? **PSIZE : std::round(g_pCursorManager->m_currentStyleInfo.size * **PSHAKE_BASE * 1.5f); // * 1.5f to accomodate for slight growth

    // we already have loaded the same theme and size
    if (manager && loadedName == name && loadedSize == size)
        return;

    // we are currently loading another theme
    if (managerFuture) {
        // in this case we don't do anything as proceeding would block until the future is done (thanks cpp apis)
        // we just skip the update, but when retrieving the future we check again and then these changes will be loaded

        Debug::log(LOG, "Skipping hyprcursor theme reload for dynamic cursors because one is already being loaded");
        return;
    }

    style = Hyprcursor::SCursorStyleInfo { size };

    loadedSize = size;
    loadedName = name;

    Debug::log(LOG, "Creating future for loading hyprcursor theme for dynamic cursors");

    auto fut = std::async(std::launch::async, [=, style = style] () -> UP<Hyprcursor::CHyprcursorManager> {
        Debug::log(INFO, "Starting to load hyprcursor theme '{}' of size {} for dynamic cursors asynchronously ...", name, size);
        auto time = std::chrono::system_clock::now();

        auto options = Hyprcursor::SManagerOptions();
        options.logFn = hcLogger;
        options.allowDefaultFallback = true;

        auto manager = makeUnique<Hyprcursor::CHyprcursorManager>(name.empty() ? nullptr : name.c_str(), options);

        if (!manager->valid()) {
            Debug::log(ERR, "... hyprcursor for dynamic cursors failed loading theme '{}', falling back to pixelated trash.", name);
            return nullptr;
        }

        manager->loadThemeStyle(style);

        float ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time).count();
        Debug::log(INFO, "... hyprcursor for dynamic cursors loading finished, took {}ms", ms);

        return manager;
    });

    manager = nullptr; // free old manager
    managerFuture = makeUnique<std::future<UP<Hyprcursor::CHyprcursorManager>>>(std::move(fut));
}

void CHighresHandler::loadShape(const std::string& name) {
    static auto const* PFALLBACK = (Hyprlang::STRING const*) getConfig(CONFIG_HIGHRES_FALLBACK);

    if (!manager) {
        // don't show old, potentially outdated shapes
        texture = nullptr;
        buffer = nullptr;

        if (!managerFuture || managerFuture->wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;

        Debug::log(INFO, "Future for hyprcursor theme for dynamic cursors is ready, using new theme");
        manager = managerFuture->get();
        managerFuture = nullptr;

        if (!manager) return; // could've failed
        else {
            // in case someone has updated the theme again in the meantime
            update();
            if (!manager) return; // new manager could be on the way
        }
    }

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
