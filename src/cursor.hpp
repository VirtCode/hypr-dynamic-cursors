#include "globals.hpp"
#include <hyprland/src/managers/PointerManager.hpp>
#include <hyprutils/math/Vector2D.hpp>

class CDynamicCursors;

class CDynamicCursors {
  public:
    /* hook on renderSoftwareCursorsFor */
    void render(CPointerManager* pointers, SP<CMonitor> pMonitor, timespec* now, CRegion& damage, std::optional<Vector2D> overridePos);

  private:
    // calculates the current angle of the cursor
    double calculate(Vector2D* pos);
    // this is the end of the virtual stick
    Vector2D end;
};

inline std::unique_ptr<CDynamicCursors> g_pDynamicCursors;
