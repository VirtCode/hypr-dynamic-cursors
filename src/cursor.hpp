#include "globals.hpp"
#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#undef private
#include <hyprutils/math/Vector2D.hpp>

class CDynamicCursors;

class CDynamicCursors {
  public:
    /* hook on onCursorMoved */
    void onCursorMoved(CPointerManager* pointers);
    /* called on tick */
    void onTick(CPointerManager* pointers);

    /* hook on renderSoftwareCursorsFor */
    void renderSoftware(CPointerManager* pointers, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos);
    /* hook on damageIfSoftware*/
    void damageSoftware(CPointerManager* pointers);
    /* hook on renderHWCursorBuffer */
    wlr_buffer* renderHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture);
    /* hook on setHWCursorBuffer */
    bool setHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, wlr_buffer* buf);

  private:
    // current angle of the cursor in radiants
    double angle;

    // calculates the current angle of the cursor, and changes the cursor shape
    void calculate();

    // calculate the angle of the cursor if stick
    double calculateStick();
    // this is the end of the virtual stick
    Vector2D end;

    // calculate the angle of the cursor if air
    double calculateAir();
    // ring buffer of last position samples
    std::vector<Vector2D> samples;
    int samples_index = 0;
};

inline std::unique_ptr<CDynamicCursors> g_pDynamicCursors;
