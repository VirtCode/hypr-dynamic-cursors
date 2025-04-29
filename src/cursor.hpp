#include "globals.hpp"
#include <memory>

#define private public
#include <hyprland/src/managers/PointerManager.hpp>
#undef private
#include <hyprutils/math/Vector2D.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprcursor/hyprcursor.hpp>

#include "mode/ModeRotate.hpp"
#include "mode/ModeTilt.hpp"
#include "mode/ModeStretch.hpp"
#include "other/Shake.hpp"
#include "highres.hpp"

class CDynamicCursors {
  public:
    CDynamicCursors();
    ~CDynamicCursors();

    /* hook on onCursorMoved */
    void onCursorMoved(CPointerManager* pointers);
    /* called on tick */
    void onTick(CPointerManager* pointers);

    /* hook on renderSoftwareCursorsFor */
    void renderSoftware(CPointerManager* pointers, SP<CMonitor> pMonitor, const Time::steady_tp& now, CRegion& damage, std::optional<Vector2D> overridePos, bool forceRender);
    /* hook on damageIfSoftware*/
    void damageSoftware(CPointerManager* pointers);
    /* hook on renderHWCursorBuffer */
    SP<Aquamarine::IBuffer> renderHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<CTexture> texture);
    /* hook on setHWCursorBuffer */
    bool setHardware(CPointerManager* pointers, SP<CPointerManager::SMonitorPointerState> state, SP<Aquamarine::IBuffer> buf);

    /* hook on setCursorFromName */
    void setShape(const std::string& name);
    /* hook on setCursorSoftware */
    void unsetShape();
    /* hook on updateTheme */
    void updateTheme();

    /* hook on move, indicate that next onCursorMoved is actual move */
    void setMove();

    void dispatchMagnify(std::optional<int> duration, std::optional<float> size);

  private:
    SP<CEventLoopTimer> tick;

    /* hyprcursor handler for highres images */
    CHighresHandler highres;

    // current state of the cursor
    SModeResult resultMode;
    double resultShake;
    Vector2D lastPos; // used for warp compensation

    SModeResult resultShown;

    // whether we have already locked software for cursor zoom
    bool zoomSoftware = false;

    // modes
    CModeRotate rotate;
    CModeTilt tilt;
    CModeStretch stretch;

    /* returns the current mode, nullptr if none is selected */
    IMode* currentMode();
    IMode* lastMode; // used to reset the mode if it was switched (to prune stale data)

    // shake
    CShake shake;

    /* is set true if a genuine move is being performed, and will be reset to false after onCursorMoved */
    bool isMove = false;

    // calculates the current angle of the cursor, and changes the cursor shape
    void calculate(EModeUpdate type);
};

inline UP<CDynamicCursors> g_pDynamicCursors;
