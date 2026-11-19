#include "std/walk_steer.h"
#include <math.h>

int walk_steer(float from_x, float from_z, float want_x, float want_z, float reach, float jump,
               float *x, float *z)
{
    float dx = want_x - from_x, dz = want_z - from_z;
    float far = sqrtf(dx * dx + dz * dz);

    if (far <= reach || far > jump) {
        *x = want_x;
        *z = want_z;
        return 0;
    }
    *x = from_x + dx * (reach / far);
    *z = from_z + dz * (reach / far);
    return 1;
}
