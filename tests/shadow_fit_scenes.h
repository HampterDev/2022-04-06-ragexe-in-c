#ifndef TESTS_SHADOW_FIT_SCENES_H
#define TESTS_SHADOW_FIT_SCENES_H

#include "render/shadow_fit.h"
#include <stddef.h>

/* Scenes for the shadow fit's tests (render/shadow_fit.h): ground over a square map with models
   and walls as boxes, some of them blended (seen through), seen by the client's camera, whose view
   depths are cast exactly into the two depth buffers of SCENE_W by SCENE_H pixels as the
   receivers' depth pass would draw them. */

#define SCENE_W 256
#define SCENE_H 144
#define SCENE_PIXELS (SCENE_W * SCENE_H)
#define MAP 1040.0   /* the map's ground from -MAP to MAP in x and z, as wide as the widest map's */
#define LEAVES 32    /* the scene graph's leaves across the ground */
#define MAX_BOXES 64
#define SCENE_BOXES (LEAVES * LEAVES + MAX_BOXES)
#define SCENE_KINDS 5
#define PI 3.14159265358979

struct Scene {
    int ground;   /* the ground y = slope[0] x + slope[1] z + slope[2] over the map */
    double slope[3];
    int count;
    double box[MAX_BOXES][2][3];  /* models and walls */
    int blended[MAX_BOXES];       /* seen through: what lies behind shows too */
};

struct Pose {
    double eye[3];
    double axis[3][3];  /* right, down and ahead on the screen */
    double distance;
};

/* the last frame render drew: each pixel's view depth of the nearest receiver that is not blended
   and the point it sees there (0 and the eye for none), and of the nearest receiver at all and its
   point, the same where nothing blended lies before the other */
extern float depth[SCENE_PIXELS];
extern double hit[SCENE_PIXELS][3];
extern float front[SCENE_PIXELS];
extern double frontHit[SCENE_PIXELS][3];
extern const char *const scene_names[SCENE_KINDS];

/* a number from lo to hi of a fixed sequence */
double uniform(double lo, double hi);
/* the depth buffers of that frame (front, depth) for render/shadow_fit.h */
struct ShadowFitDepths scene_depths(void);
/* flat ground, a slope, a wall on flat ground, or models on it (each time others), or models on it
   half of which are blended */
void scene_make(struct Scene *s, int kind);
double ground_height(const struct Scene *s, double x, double z);
/* the boxes the receivers and casters lie in, as the scene graph hands them to the CPU's fit: the
   ground by leaf, then each model's own; at most SCENE_BOXES */
size_t scene_boxes(const struct Scene *s, struct ShadowFitBox *out);
/* looking at focus from distance, turned by yaw and looking down by pitch (radians, +y down) */
void pose_look(struct Pose *p, const double *focus, double yaw, double pitch, double distance);
/* the client's camera for the pose: the projection as renderer_set_projection makes it */
void pose_camera(const struct Pose *p, int width, int height, struct ShadowFitCamera *cam);
void scene_render(const struct Scene *s, const struct Pose *p, const struct ShadowFitCamera *cam);
/* a light from above, or a camera somewhere over the scene */
void random_light(float *light);
void random_pose(const struct Scene *s, struct Pose *p);

#endif
