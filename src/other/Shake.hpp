#include <hyprutils/math/Vector2D.hpp>
#include <vector>

using namespace Hyprutils::Math;

class CShake {
  public:
    /* calculates the new zoom factor for the current pos */
    double update(Vector2D pos);

  private:
    /* tracks the global software lock issued by cursor shaking */
    bool software = false;

    /* ringbuffer for last samples */
    std::vector<Vector2D> samples;
    /* we also store the distance for each sample to the last, so we do only compute this once */
    std::vector<double> samples_distance;
    int samples_index = 0;
};
