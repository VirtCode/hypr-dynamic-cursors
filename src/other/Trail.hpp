#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <hyprland/src/pointer/PointerManager.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <deque>
#include <chrono>

#include "../mode/utils.hpp"

using namespace std::chrono;

class CTrail {
  public:
    struct TrailPoint {
        Vector2D                          pos;
        SP<Render::ITexture>              tex;
        SP<Render::ITexture>              highres_tex;
        Vector2D                          highHostpotFrac;
        Vector2D                          HighSize;
        SModeResult                       result;
        Vector2D                          size;
        Vector2D                          hotspot;
        float                             imageScale;
        high_resolution_clock::time_point timestamp;

        float age(void) const {
            return duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - timestamp).count();
        }
    };

    float timeSinceLastPush(void);
    bool hasChanged(Vector2D pos, SModeResult& given_result);

    bool push(
        Vector2D pos, const Pointer::CPointerManager::SCursorImage& img, SModeResult& given_result, const SP<Render::ITexture>  highres_tex, Vector2D highHotspotFrac, Vector2D highSize
    );
    
    const std::deque<TrailPoint>& get() const {
        return samples;
    }

    /* called when a cursor warp has happened (avoids inaccurate damage bounds / trails) */
    void warp(void);

  private:
    high_resolution_clock::time_point last_push_time = high_resolution_clock::now();
    std::deque<TrailPoint> samples;
};
