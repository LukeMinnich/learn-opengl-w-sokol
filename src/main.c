#include "light.glsl.h"
#include "scene.glsl.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"

#include "handmade_math.h"
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdlib.h>

typedef float f32;
typedef uint16_t u16;
typedef size_t usize;
typedef unsigned char byte;

#define WIDTH 800
#define HEIGHT 600
#define FOV_MAX 45.f

typedef struct {
	HMM_Vec3 pos;
	HMM_Vec2 tex;
	HMM_Vec3 norm;
} Vertex;

static const Vertex cube_vertices[] = {
	{ { .X = -0.5f, -0.5f, -0.5f }, { .U = 0.0f, 0.0f }, {.X =  0.0f,  0.0f, -1.0f} },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .U = 1.0f, 0.0f }, {.X =  0.0f,  0.0f, -1.0f} },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .U = 1.0f, 1.0f }, {.X =  0.0f,  0.0f, -1.0f} },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .U = 1.0f, 1.0f }, {.X =  0.0f,  0.0f, -1.0f} },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X =  0.0f,  0.0f, -1.0f} },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .U = 0.0f, 0.0f }, {.X =  0.0f,  0.0f, -1.0f} },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .U = 0.0f, 0.0f }, {.X =  0.0f,  0.0f,  1.0f} },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X =  0.0f,  0.0f,  1.0f} },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .U = 1.0f, 1.0f }, {.X =  0.0f,  0.0f,  1.0f} },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .U = 1.0f, 1.0f }, {.X =  0.0f,  0.0f,  1.0f} },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .U = 0.0f, 1.0f }, {.X =  0.0f,  0.0f,  1.0f} },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .U = 0.0f, 0.0f }, {.X =  0.0f,  0.0f,  1.0f} },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X = -1.0f,  0.0f,  0.0f} },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .U = 1.0f, 1.0f }, {.X = -1.0f,  0.0f,  0.0f} },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X = -1.0f,  0.0f,  0.0f} },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X = -1.0f,  0.0f,  0.0f} },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .U = 0.0f, 0.0f }, {.X = -1.0f,  0.0f,  0.0f} },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X = -1.0f,  0.0f,  0.0f} },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X =  1.0f,  0.0f,  0.0f} },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .U = 1.0f, 1.0f }, {.X =  1.0f,  0.0f,  0.0f} },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X =  1.0f,  0.0f,  0.0f} },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X =  1.0f,  0.0f,  0.0f} },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .U = 0.0f, 0.0f }, {.X =  1.0f,  0.0f,  0.0f} },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X =  1.0f,  0.0f,  0.0f} },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X =  0.0f, -1.0f,  0.0f} },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .U = 1.0f, 1.0f }, {.X =  0.0f, -1.0f,  0.0f} },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X =  0.0f, -1.0f,  0.0f} },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X =  0.0f, -1.0f,  0.0f} },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .U = 0.0f, 0.0f }, {.X =  0.0f, -1.0f,  0.0f} },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X =  0.0f, -1.0f,  0.0f} },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X =  0.0f,  1.0f,  0.0f} },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .U = 1.0f, 1.0f }, {.X =  0.0f,  1.0f,  0.0f} },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X =  0.0f,  1.0f,  0.0f} },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .U = 1.0f, 0.0f }, {.X =  0.0f,  1.0f,  0.0f} },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .U = 0.0f, 0.0f }, {.X =  0.0f,  1.0f,  0.0f} },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .U = 0.0f, 1.0f }, {.X =  0.0f,  1.0f,  0.0f} },
};

static const HMM_Vec3 cube_positions[] = {
	{ .X =  0.0f,  0.0f,   0.0f },
	{ .X =  2.0f,  5.0f, -15.0f },
	{ .X = -1.5f, -2.2f,  -2.5f },
	{ .X = -3.8f, -2.0f, -12.3f },
	{ .X =  2.4f, -0.4f,  -3.5f },
	{ .X = -1.7f,  3.0f,  -7.5f },
	{ .X =  1.3f, -2.0f,  -2.5f },
	{ .X =  1.5f,  2.0f,  -2.5f },
	{ .X =  1.5f,  0.2f,  -1.5f },
	{ .X = -1.3f,  1.0f,  -1.5f },
};

typedef struct {
	f32 pitch;
	f32 yaw;
	f32 fov;
	HMM_Vec3 pos;
} Camera;

#define CAMERA_UP ((HMM_Vec3){ .X = 0.f, .Y = 1.f, .Z = 0.f })

typedef struct {
	sg_pipeline lighting_target_pipeline;
	sg_pipeline lighting_source_pipeline;
	sg_bindings lighting_target_bindings;
	sg_bindings lighting_source_bindings;
	sg_pass_action pass_action;
	byte *image_data;
	Camera camera;
	f32 delta_time;
	bool window_focused;
} AppState;

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static const DirLight_t dir_lights[] = {
	{
		.direction = { .X = -0.2f , -1.f  , -0.3f  },
		.ambient   = { .R =  0.05f,  0.05f,  0.05f },
		.diffuse   = { .R =  0.4f ,  0.4f ,  0.4f  },
		.specular  = { .R =  0.5f ,  0.5f ,  0.5f  },
	},
};

static const HMM_Vec3 point_light_positions[] = {
	{ .X =  0.7f,  0.2f,   2.0f },
	{ .X =  2.3f, -3.3f,  -4.0f },
	{ .X = -4.0f,  2.0f, -12.0f },
	{ .X =  0.0f,  0.0f,  -3.0f },
};

static const PointLight_t point_lights[] = {
	{
		.position  = point_light_positions[0],
		.ambient   = { .R =  0.05f,  0.05f,  0.05f },
		.diffuse   = { .R =  0.8f ,  0.8f ,  0.8f  },
		.specular  = { .R =  1.0f ,  1.0f ,  1.0f  },
		.constant  = 1.f,
		.linear    = 0.09f,
		.quadratic = 0.032f,
	}, {
		.position  = point_light_positions[1],
		.ambient   = { .R =  0.05f },
		.diffuse   = { .R =  0.8f  },
		.specular  = { .R =  1.0f  },
		.constant  = 1.f,
		.linear    = 0.09f,
		.quadratic = 0.032f,
	}, {
		.position  = point_light_positions[2],
		.ambient   = { .G = 0.05f },
		.diffuse   = { .G = 0.8f  },
		.specular  = { .G = 1.0f  },
		.constant  = 1.f,
		.linear    = 0.09f,
		.quadratic = 0.032f,
	}, {
		.position  = point_light_positions[3],
		.ambient   = { .B = 0.05f },
		.diffuse   = { .B = 0.8f  },
		.specular  = { .B = 1.0f  },
		.constant  = 1.f,
		.linear    = 0.09f,
		.quadratic = 0.032f,
	},
};

static HMM_Vec3
cam_direction_from_pitch_yaw(
	float pitch,
	float yaw
) {
	HMM_Vec3 dir = {
		.X = cosf(HMM_AngleDeg(yaw)) * cosf(HMM_AngleDeg(pitch)),
		.Y = sinf(HMM_AngleDeg(pitch)),
		.Z = sinf(HMM_AngleDeg(yaw)) * cosf(HMM_AngleDeg(pitch)),
	};
	return HMM_Norm(dir);
}

static void
state_init(
	AppState *state
) {
	stbi_set_flip_vertically_on_load(true);
	int w, h, n_channels;
	byte *diffuse_data = stbi_load("assets/container2.png", &w, &h, &n_channels, 4);
	assert(diffuse_data);
	sg_image diffuse_img = sg_make_image(&(sg_image_desc){
		.width = w,
		.height = h,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data.mip_levels[0] = {
			.ptr = diffuse_data,
			.size = (size_t)(w * h * 4),
		},
	});
	stbi_image_free(diffuse_data);

	byte *specular_data = stbi_load("assets/container2_specular.png", &w, &h, &n_channels, 4);
	assert(specular_data);
	sg_image specular_img = sg_make_image(&(sg_image_desc){
		.width = w,
		.height = h,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data.mip_levels[0] = {
			.ptr = specular_data,
			.size = (size_t)(w * h * 4),
		},
	});
	stbi_image_free(specular_data);

	sg_buffer cube_vertex_bufffer = sg_make_buffer(&(sg_buffer_desc){
		.usage.vertex_buffer = true,
		.data = SG_RANGE(cube_vertices),
		.label = "vertex-buffer",
	});

	*state = (AppState){
		.lighting_target_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(scene_shader_desc(sg_query_backend())),
			.layout = {
				.attrs = {
					[ATTR_scene_aPos].format = SG_VERTEXFORMAT_FLOAT3,
					[ATTR_scene_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2,
					[ATTR_scene_aNormal].format = SG_VERTEXFORMAT_FLOAT3,
				},
			},
			.label = "lighting-target-pipeline",
			.depth = {
				.write_enabled = true,
				.compare = SG_COMPAREFUNC_LESS_EQUAL,
			}
		}),
		.lighting_source_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(light_shader_desc(sg_query_backend())),
			.layout = {
				.attrs = {
					[ATTR_light_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				},
				.buffers = {
					[ATTR_light_aPos].stride = sizeof(Vertex),
				},
			},
			.label = "lighting-source-pipeline",
			.depth = {
				.write_enabled = true,
				.compare = SG_COMPAREFUNC_LESS_EQUAL,
			}
		}),
		.lighting_target_bindings.vertex_buffers[0] = cube_vertex_bufffer,
		.lighting_target_bindings.views[VIEW_diffuseTexture] = sg_make_view(&(sg_view_desc){
			.texture = {.image = diffuse_img },
			.label = "diffuse-texture",
		}),
		.lighting_target_bindings.views[VIEW_specularTexture] = sg_make_view(&(sg_view_desc){
			.texture = {.image = specular_img },
			.label = "specular-texture",
		}),
		.lighting_target_bindings.views[VIEW_in_dir_lights] = sg_make_view(&(sg_view_desc){
			.storage_buffer =  {
				.buffer = sg_make_buffer(&(sg_buffer_desc){
					.usage.storage_buffer = true,
					.data = SG_RANGE(dir_lights),
					.label = "directional-lights",
				}),
			},
			.label = "directional-lights-storage",
		}),
		.lighting_target_bindings.views[VIEW_in_point_lights] = sg_make_view(&(sg_view_desc){
			.storage_buffer =  {
				.buffer = sg_make_buffer(&(sg_buffer_desc){
					.usage.storage_buffer = true,
					.data = SG_RANGE(point_lights),
					.label = "point-lights",
				}),
			},
			.label = "point-lights-storage",
		}),
		.lighting_target_bindings.samplers[SMP_boxSampler] = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.label = "box-sampler"
		}),
		.lighting_source_bindings.vertex_buffers[0] = cube_vertex_bufffer,
		.pass_action = (sg_pass_action) {
			.colors[0] = {
				.load_action = SG_LOADACTION_CLEAR,
				.clear_value = { 0.1f, 0.1f, 0.1f, 1.0f },
			}
		},
		.camera.yaw = -90.f,
		.camera.pos = HMM_V3(0.f, 0.f,  3.f),
		.camera.fov = FOV_MAX,
		.window_focused = true,
	};
}

static void
app_frame(
	void *state_
) {
	AppState *state = state_;

	uint64_t current_frame = stm_now();
	static uint64_t last_frame = 0;
	state->delta_time = stm_sec(stm_diff(current_frame, last_frame));
	last_frame = current_frame;

	sg_begin_pass(&(sg_pass){
		.action = state->pass_action,
		.swapchain = sglue_swapchain()
	});

	/* Draw textured boxes */
	sg_apply_pipeline(state->lighting_target_pipeline);
	sg_apply_bindings(&state->lighting_target_bindings);

	scene_material_t scene_material = {
		.mat_shininess = 32.f,
	};
	sg_apply_uniforms(UB_scene_material, &SG_RANGE(scene_material));

	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(state->camera.pitch, state->camera.yaw);
	scene_vs_params_t scene_vs_params = {
		.view       = HMM_LookAt_RH(state->camera.pos, HMM_Add(state->camera.pos, camera_front), CAMERA_UP),
		.projection = HMM_Perspective_RH_NO(HMM_AngleDeg(state->camera.fov), (f32)WIDTH / (f32)HEIGHT, 0.1f, 100.f),
	};
	for (usize i = 0; i < countof(cube_positions); ++i) {
		scene_vs_params.model = HMM_Mul(HMM_Translate(cube_positions[i]),
		                                HMM_Rotate_RH(HMM_AngleDeg(20.f * i), HMM_V3(1.f, 0.3f, 0.5f)));
		scene_vs_params.normal = HMM_Transpose(HMM_InvGeneral(scene_vs_params.model));
		sg_apply_uniforms(UB_scene_vs_params, &SG_RANGE(scene_vs_params));
		scene_fs_params_t scene_fs_params = {
			.viewPos = state->camera.pos,
			.n_dir_lights = countof(dir_lights),
			.n_point_lights = countof(point_lights),
		};
		sg_apply_uniforms(UB_scene_fs_params, &SG_RANGE(scene_fs_params));
		sg_draw(0, countof(cube_vertices), 1);
	}

	/* Draw point light boxes */
	sg_apply_pipeline(state->lighting_source_pipeline);
	sg_apply_bindings(&state->lighting_source_bindings);

	light_vs_params_t light_vs_params = {
		.view = scene_vs_params.view,
		.projection = scene_vs_params.projection,
	};
	for (usize i = 0; i < countof(point_lights); ++i){
		light_vs_params.model = HMM_Mul(HMM_Translate(point_lights[i].position),
		                        HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f)));
		sg_apply_uniforms(UB_light_vs_params, &SG_RANGE(light_vs_params));
		light_fs_params_t light_fs_params = {
			.light_color = point_lights[i].specular,
		};
		sg_apply_uniforms(UB_light_fs_params, &SG_RANGE(light_fs_params));
		sg_draw(0, countof(cube_vertices), 1);
	}

	sg_end_pass();
	sg_commit();
}

static void
app_handle_event(
	const sapp_event *event,
	void *state_
) {
	AppState *state = state_;
	if (!state->window_focused) {
		if (SAPP_EVENTTYPE_FOCUSED == event->type) {
			state->window_focused = true;
		}
		return;
	}
	if (SAPP_EVENTTYPE_UNFOCUSED == event->type) {
		state->window_focused = false;
	}
	if (   SAPP_EVENTTYPE_CHAR == event->type
	    && (   'q' == event->char_code
	        || 'Q' == event->char_code)) {
		sapp_quit();
	}
	f32 camera_speed = 10.f * state->delta_time;
	if (SAPP_EVENTTYPE_CHAR == event->type) {
		HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(state->camera.pitch, state->camera.yaw);
		if ('w' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(camera_front, camera_speed);
			state->camera.pos = HMM_Add(state->camera.pos, delta);
		}
		if ('s' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(camera_front, camera_speed);
			state->camera.pos = HMM_Sub(state->camera.pos, delta);
		}
		if ('a' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(HMM_Norm(HMM_Cross(camera_front, CAMERA_UP)), camera_speed);
			state->camera.pos = HMM_Sub(state->camera.pos, delta);
		}
		if ('d' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(HMM_Norm(HMM_Cross(camera_front, CAMERA_UP)), camera_speed);
			state->camera.pos = HMM_Add(state->camera.pos, delta);
		}
	}
	if (SAPP_EVENTTYPE_MOUSE_MOVE == event->type) {
		f32 sensitivity = 0.1f;
		f32 d_yaw   =  event->mouse_dx * sensitivity;
		f32 d_pitch = -event->mouse_dy * sensitivity; // positive `mouse_dy` is screen downward
		state->camera.yaw += d_yaw;
		state->camera.pitch = HMM_Clamp(-89.f, state->camera.pitch + d_pitch, 89.f);
	}
	if (SAPP_EVENTTYPE_MOUSE_SCROLL == event->type) {
		state->camera.fov = HMM_Clamp(1.f, state->camera.fov - event->scroll_y, FOV_MAX);
	}
}

static void
app_init(
	void *state_
) {
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});
	stm_setup();
	state_init((AppState *)state_);
	sapp_lock_mouse(true);
}

static void
app_cleanup(
	void *state_
) {
	sg_shutdown();
	free(state_);
}

sapp_desc sokol_main(
	int argc,
	char* argv[]
) {
	(void)argc;
	(void)argv;
	AppState *state = malloc(sizeof(*state));
	return (sapp_desc){
		.init_userdata_cb = app_init,
		.frame_userdata_cb = app_frame,
		.cleanup_userdata_cb = app_cleanup,
		.user_data = state,
		.event_userdata_cb = app_handle_event,
		.width = WIDTH,
		.height = HEIGHT,
		.window_title = "Triangle",
		.icon.sokol_default = true,
		.logger.func = slog_func,
	};
}
