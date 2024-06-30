#include "globals.hpp"

#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#undef private
#include <hyprutils/math/Vector2D.hpp>

#include "mode/ModeRotate.hpp"
#include "mode/ModeTilt.hpp"
#include "other/Shake.hpp"

class CDynamicCursors {
  public:
    /* hook on onCursorMoved */
    void onCursorMoved(CPointerManager* pointers);
    /* called on tick */
    void onTick(CPointerManager* pointers);
    /* called before render */
    void beforeRender(CPointerManager* pointers);

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
    // current zoom value of the cursor
    double zoom = 1;

    // whether we have already locked software for cursor zoom
    bool zoomSoftware = false;

    // modes
    CModeRotate rotate;
    CModeTilt tilt;
    /* returns the current mode, nullptr if none is selected */
    IMode* currentMode();

    // shake
    CShake shake;

    // calculates the current angle of the cursor, and changes the cursor shape
    void calculate(EModeUpdate type);
};

inline std::unique_ptr<CDynamicCursors> g_pDynamicCursors;
