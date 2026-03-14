#include "scene.glsl.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"

#include <stdlib.h>

typedef float f32;

static const f32 vertices[] = {
	-0.5f, -0.5f, 0.0f,
	 0.5f, -0.5f, 0.0f,
	 0.0f,  0.5f, 0.0f,
};

typedef struct {
	sg_pipeline pipeline;
	sg_bindings bindings;
	sg_pass_action pass_action;
} AppState;

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static void
state_init(
	AppState *state
) {
	*state = (AppState){
		.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(scene_shader_desc(sg_query_backend())),
			.layout = {
				.attrs = {
					[ATTR_scene_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				}
			},
			.label = "scene-pipeline"
		}),
		.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
			.data = SG_RANGE(vertices),
			.label = "vertex-buffer",
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
	sg_draw(0, countof(vertices) / 3, 1);
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
		.width = 640,
		.height = 480,
		.window_title = "Triangle",
		.icon.sokol_default = true,
		.logger.func = slog_func,
	};
}
