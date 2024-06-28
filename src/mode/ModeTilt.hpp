#include "Mode.hpp"
#include <hyprutils/math/Vector2D.hpp>
#include <vector>

class CModeTilt : public IMode {
  public:
    virtual EModeUpdate strategy();
    virtual double update(Vector2D pos);

  private:

    // ring buffer of last position samples
    std::vector<Vector2D> samples;
    int samples_index = 0;

};
