#ifndef RENDER_H
#define RENDER_H

#include "primitives.h"

#include "sokol_gfx.h"

/* MSAA */
#define OFFSCREEN_SAMPLE_COUNT 4
#define DISPLAY_SAMPLE_COUNT   4
#define SHADOW_SAMPLE_COUNT    1

#define ORIGIN ((HMM_Vec3){0})
#define X_AXIS ((HMM_Vec3){ .X = 1.f })
#define Y_AXIS ((HMM_Vec3){ .Y = 1.f })
#define Z_AXIS ((HMM_Vec3){ .Z = 1.f })

typedef struct {
	struct {
		sg_view view;
		sg_buffer buffer;
		usize len;
	} directional;
	struct {
		sg_view view;
		sg_buffer buffer;
		usize len;
	} point;
} Lighting;

#endif // RENDER_H
