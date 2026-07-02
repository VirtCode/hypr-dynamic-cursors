#include <any>    // IWYU pragma: keep
#include <chrono> // IWYU pragma: keep
#define private public
#include <hyprland/src/pointer/cursor/CursorManager.hpp>
#undef private

#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprlang.hpp>
#include <hyprcursor/hyprcursor.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <chrono>
#include <cmath>
#include <future>

#include "highres.hpp"
#include "config/ConfigManager.hpp"

CHighresHandler::CHighresHandler() {
    // load stuff on creation
    update();

    // and reload on config reload
    static const auto LISTENER = Event::bus()->m_events.config.reloaded.listen([&]() -> void { update(); });
}

static void hcLogger(enum eHyprcursorLogLevel level, char* message) {
    if (level == HC_LOG_TRACE)
        return;
    Log::logger->log(Log::INFO, "[hc (dynamic)] {}", message);
}

void CHighresHandler::update() {
    static auto PUSEHYPRCURSOR = CConfigValue<Hyprlang::INT>("cursor:enable_hyprcursor");

    if (!g_pConfigHandler->isEnabled() || !CONFIG(highresEnabled) || !*PUSEHYPRCURSOR || !CONFIG(shakeEnabled)) {
        // free manager if no longer enabled
        if (manager) {
            manager = nullptr;
            texture = nullptr;
            buffer  = nullptr;
        }

        return;
    }

    std::string  name = Pointer::Cursor::mgr()->m_theme;
    unsigned int size = CONFIG(highresSize) != -1 ?
        CONFIG(highresSize) :
        std::round(Pointer::Cursor::mgr()->m_currentStyleInfo.size * CONFIG(shakeBase) * 1.5f); // * 1.5f to accommodate for slight growth

    // we already have loaded the same theme and size
    if (manager && loadedName == name && loadedSize == size)
        return;

    // we are currently loading another theme
    if (managerFuture) {
        // in this case we don't do anything as proceeding would block until the future is done (thanks cpp apis)
        // we just skip the update, but when retrieving the future we check again and then these changes will be loaded

        Log::logger->log(Log::INFO, "Skipping hyprcursor theme reload for dynamic cursors because one is already being loaded");
        return;
    }

    style = Hyprcursor::SCursorStyleInfo{size};

    loadedSize = size;
    loadedName = name;

    Log::logger->log(Log::INFO, "Creating future for loading hyprcursor theme for dynamic cursors");

    auto fut = std::async(std::launch::async, [=, style = style]() -> UP<Hyprcursor::CHyprcursorManager> {
        Log::logger->log(Log::INFO, "Starting to load hyprcursor theme '{}' of size {} for dynamic cursors asynchronously ...", name, size);
        auto time = std::chrono::system_clock::now();

        auto options                 = Hyprcursor::SManagerOptions();
        options.logFn                = hcLogger;
        options.allowDefaultFallback = true;

        auto manager = makeUnique<Hyprcursor::CHyprcursorManager>(name.empty() ? nullptr : name.c_str(), options);

        if (!manager->valid()) {
            Log::logger->log(Log::ERR, "... hyprcursor for dynamic cursors failed loading theme '{}', falling back to pixelated trash.", name);
            return nullptr;
        }

        manager->loadThemeStyle(style);

        float ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time).count();
        Log::logger->log(Log::INFO, "... hyprcursor for dynamic cursors loading finished, took {}ms", ms);

        return manager;
    });

    manager       = nullptr; // free old manager
    managerFuture = makeUnique<std::future<UP<Hyprcursor::CHyprcursorManager>>>(std::move(fut));
}

void CHighresHandler::loadShape(const std::string& name) {
    if (!manager) {
        // don't show old, potentially outdated shapes
        texture = nullptr;
        buffer  = nullptr;

        if (!managerFuture || managerFuture->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;

        Log::logger->log(Log::INFO, "Future for hyprcursor theme for dynamic cursors is ready, using new theme");
        manager       = managerFuture->get();
        managerFuture = nullptr;

        if (!manager)
            return; // could've failed
        else {
            // in case someone has updated the theme again in the meantime
            update();
            if (!manager)
                return; // new manager could be on the way
        }
    }

    Hyprcursor::SCursorShapeData shape = manager->getShape(name.c_str(), style);

    // try load fallback image
    if (shape.images.size() == 0) {
        shape = manager->getShape(CONFIG(highresFallback).c_str(), style);

        if (shape.images.size() == 0) {
            Log::logger->log(Log::WARN, "Failed to load fallback shape {}, for shape {}!", CONFIG(highresFallback), name);

            texture = nullptr;
            buffer  = nullptr;
            return;
        }
    }

    buffer = makeShared<Pointer::Cursor::CCursorBuffer>(shape.images[0].surface, Vector2D{shape.images[0].size, shape.images[0].size},
                                                        Vector2D{shape.images[0].hotspotX, shape.images[0].hotspotY});

    texture = g_pHyprRenderer->createTexture(SP<Aquamarine::IBuffer>(buffer));
}

SP<Render::ITexture> CHighresHandler::getTexture() {
    return texture;
}

SP<Pointer::Cursor::CCursorBuffer> CHighresHandler::getBuffer() {
    return buffer;
}
