#include "Mode.hpp"
#include <hyprutils/math/Vector2D.hpp>

/*
this modes simulates a stick being dragged on one end
this results in a rotating mouse cursor
*/
class CModeRotate : public IMode {
  public:
    virtual EModeUpdate strategy();
    virtual SModeResult update(Vector2D pos);

  private:

    // end of the simulated stick
    Vector2D end;

};
