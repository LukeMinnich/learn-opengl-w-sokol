#include "scene.glsl.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"

#include "handmade_math.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdlib.h>

typedef float f32;
typedef uint16_t u16;
typedef size_t usize;
typedef unsigned char byte;

#define WIDTH 800
#define HEIGHT 600

static const f32 vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

static const HMM_Vec3 cube_positions[] = {
    { .X =  0.0f, .Y =  0.0f, .Z =   0.0f },
    { .X =  2.0f, .Y =  5.0f, .Z = -15.0f },
    { .X = -1.5f, .Y = -2.2f, .Z =  -2.5f },
    { .X = -3.8f, .Y = -2.0f, .Z = -12.3f },
    { .X =  2.4f, .Y = -0.4f, .Z =  -3.5f },
    { .X = -1.7f, .Y =  3.0f, .Z =  -7.5f },
    { .X =  1.3f, .Y = -2.0f, .Z =  -2.5f },
    { .X =  1.5f, .Y =  2.0f, .Z =  -2.5f },
    { .X =  1.5f, .Y =  0.2f, .Z =  -1.5f },
    { .X = -1.3f, .Y =  1.0f, .Z =  -1.5f },
  };

typedef struct {
	sg_pipeline pipeline;
	sg_bindings bindings;
	sg_pass_action pass_action;
	byte *image_data;
} AppState;

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static void
state_init(
	AppState *state
) {
	stbi_set_flip_vertically_on_load(true);
	int w, h, n_channels;
	byte *box_data = stbi_load("assets/container.jpg", &w, &h, &n_channels, 4);
	assert(box_data);
	sg_image box_img = sg_make_image(&(sg_image_desc){
		.width = w,
		.height = h,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data.mip_levels[0] = {
			.ptr = box_data,
			.size = (size_t)(w * h * 4),
		},
	});
	stbi_image_free(box_data);
	byte *face_data = stbi_load("assets/awesomeface.png", &w, &h, &n_channels, 4);
	assert(face_data);
	sg_image face_img = sg_make_image(&(sg_image_desc){
		.width = w,
		.height = h,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data.mip_levels[0] = {
			.ptr = face_data,
			.size = (size_t)(w * h * 4),
		},
	});
	stbi_image_free(face_data);

	*state = (AppState){
		.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(scene_shader_desc(sg_query_backend())),
			.layout = {
				.attrs = {
					[ATTR_scene_aPos].format = SG_VERTEXFORMAT_FLOAT3,
					[ATTR_scene_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2,
				}
			},
			.label = "scene-pipeline",
			.depth = {
				.write_enabled = true,
				.compare = SG_COMPAREFUNC_LESS_EQUAL,
			}
		}),
		.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.data = SG_RANGE(vertices),
			.label = "vertex-buffer",
		}),
		.bindings.views[VIEW_boxTexture] = sg_make_view(&(sg_view_desc){
			.texture = {.image = box_img },
			.label = "box-texture",
		}),
		.bindings.samplers[SMP_boxSampler] = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.label = "box-sampler"
		}),
		.bindings.views[VIEW_faceTexture] = sg_make_view(&(sg_view_desc){
			.texture = {.image = face_img },
			.label = "face-texture",
		}),
		.bindings.samplers[SMP_faceSampler] = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.label = "face-sampler"
		}),
		.pass_action = (sg_pass_action) {
			.colors[0] = {
				.load_action = SG_LOADACTION_CLEAR,
				.clear_value = { 0.2f, 0.3f, 0.3f, 1.0f },
			}
		},
	};
}

static void
app_frame(
	void *state_
) {
	AppState *state = state_;
	sg_begin_pass(&(sg_pass){
		.action = state->pass_action,
		.swapchain = sglue_swapchain()
	});
	sg_apply_pipeline(state->pipeline);
	sg_apply_bindings(&state->bindings);

	scene_vs_params_t vs_params = {
		.view       = HMM_Translate(HMM_V3(0.f, 0.f, -3.f)),
		.projection = HMM_Perspective_RH_NO(HMM_AngleDeg(45.f), (f32)WIDTH / (f32)HEIGHT, 0.1f, 100.f),
	};
	for (usize i = 0; i < countof(cube_positions); ++i) {
		vs_params.model = HMM_Mul(HMM_Translate(cube_positions[i]),
		                          HMM_Rotate_RH(HMM_AngleDeg(20.f * i), HMM_V3(1.f, 0.3f, 0.5f)));
		sg_apply_uniforms(UB_scene_vs_params, &SG_RANGE(vs_params));
		sg_draw(0, countof(vertices) / 5, 1);
	}

	sg_end_pass();
	sg_commit();
}

static void
app_handle_event(
	const sapp_event *event
) {
	if (   SAPP_EVENTTYPE_CHAR == event->type
	    && (   'q' == event->char_code
	        || 'Q' == event->char_code)) {
		sapp_quit();
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
		.event_cb = app_handle_event,
		.width = WIDTH,
		.height = HEIGHT,
		.window_title = "Triangle",
		.icon.sokol_default = true,
		.logger.func = slog_func,
	};
}
