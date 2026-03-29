#ifndef CAMERA_H
#define CAMERA_H

#include "primitives.h"
#include "render.h"

#include "handmade_math.h"

#define FOV_MAX 60.f
#define PERSPECTIVE_NEAR 0.01f
#define ORTHOGRAPHIC_NEAR 0.f
#define FAR 100.f

#define CAMERA_UP Y_AXIS

typedef struct {
	HMM_Vec3 pos;
	f32 pitch;
	f32 yaw;
	f32 fov;
} Camera;

#endif // CAMERA_H
