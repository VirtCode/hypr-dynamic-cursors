#include <hyprutils/math/Vector2D.hpp>
#include <vector>

#define IPC_SHAKE_START "shakestart"
#define IPC_SHAKE_UPDATE "shakeupdate"
#define IPC_SHAKE_END "shakeend"

using namespace Hyprutils::Math;

class CShake {
  public:
    /* calculates the new zoom factor for the current pos */
    double update(Vector2D pos);

  private:
    /* tracks whether the current shake has already been announced in the ipc */
    bool ipc = false;

    /* stores last measured diagonal */
    float diagonal = 0;

    /* ringbuffer for last samples */
    std::vector<Vector2D> samples;
    /* we also store the distance for each sample to the last, so we do only compute this once */
    std::vector<double> samples_distance;
    int samples_index = 0;
};
