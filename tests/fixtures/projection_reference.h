#ifndef TEST_PROJECTION_REFERENCE_H
#define TEST_PROJECTION_REFERENCE_H
struct Renderer;
struct TexDrawVertex;
float reference_CRenderer_ScreenScaleAtRhw(struct Renderer *, float);
float reference_CRenderer_GetScaleX(struct Renderer *);
float reference_Renderer_ProjectZ(float);
void reference_CRenderer_ProjectPoint(struct Renderer *, const float *, const float *, float *, float *, float *);
void reference_Renderer_ProjectCentred(struct Renderer *, const float *, const float *, float *, float *, float *, float, float);
void reference_CRenderer_ProjectVertex(struct Renderer *, const float *, const float *, struct TexDrawVertex *);
void reference_CRenderer_ProjectMultiTexVertex(struct Renderer *, const float *, const float *, float *);
void reference_CRenderer_CursorRay(struct Renderer *, int, int, const float *, const float *, float *);
#endif
