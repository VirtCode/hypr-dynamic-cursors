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

    // calculates the current angle of the cursor, returns whether the angle has changed
    bool calculate(Vector2D* pos);
    // this is the end of the virtual stick
    Vector2D end;
};

inline std::unique_ptr<CDynamicCursors> g_pDynamicCursors;
