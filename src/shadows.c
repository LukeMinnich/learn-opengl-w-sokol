#include "alias.h"
#include "camera.h"
#include "mesh.glsl.h"
#include "mesh.h"
#include "render.h"

#include "handmade_math.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"

#define SHADOW_MAP_SZ 1024
#define DEBUG_SZ       200

static struct {
	Mesh tetrahedron_mesh;
	Mesh plane_mesh;
	struct {
		sg_view depth_attachment;
		sg_image depth_img;
		sg_pipeline pip;
	} shadow;
	struct {
		sg_pipeline pip;
		sg_view shadow_map_tex_view;
		sg_sampler shadow_sampler;
	} display;
	struct {
		sg_pipeline pip;
		sg_bindings bind;
	} dbg;
	Camera camera;
	bool animation_paused;
	bool window_focused;
	f32 delta_time;
} state = {0};

static void
init(
	void
) {
	sapp_lock_mouse(true);
	stm_setup();
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});

	state.camera = (Camera){
		.fov = FOV_MAX,
		.pitch = -25.f,
		.yaw = -135.f,
		.pos = HMM_V3(5.0f, 5.0f, 5.0f),
	};
	state.window_focused = true;

	state.shadow.depth_img = sg_make_image(&(sg_image_desc){
		.usage.depth_stencil_attachment = true,
		.width = SHADOW_MAP_SZ,
		.height = SHADOW_MAP_SZ,
		.pixel_format = SG_PIXELFORMAT_DEPTH,
		.sample_count = SHADOW_SAMPLE_COUNT,
		.label = "shadow-map",
	});

	state.display.shadow_map_tex_view = sg_make_view(&(sg_view_desc){
		.texture = { .image = state.shadow.depth_img },
		.label = "shadow-map-tex-view",
	});

	state.shadow.depth_attachment = sg_make_view(&(sg_view_desc){
		.depth_stencil_attachment = { .image = state.shadow.depth_img },
		.label = "shadow-map-depth-stencil-view",
	});

	state.display.shadow_sampler = sg_make_sampler(&(sg_sampler_desc){
		.min_filter = SG_FILTER_LINEAR,
		.mag_filter = SG_FILTER_LINEAR,
		.wrap_u = SG_WRAP_CLAMP_TO_BORDER,
		.wrap_v = SG_WRAP_CLAMP_TO_BORDER,
		.border_color = SG_BORDERCOLOR_OPAQUE_BLACK,
		.compare = SG_COMPAREFUNC_LESS,
		.label = "shadow-sampler",
	});

	mesh_gen_primitive_tetrahedron(&state.tetrahedron_mesh);
	mesh_gen_primitive_plane(&state.plane_mesh);

	mesh_shadow_pipeline(&state.shadow.pip);
	mesh_display_pipeline(&state.display.pip);

	// a vertex buffer, pipeline and sampler to render a debug visualization of the shadow map
	float dbg_vertices[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
	sg_buffer dbg_vbuf = sg_make_buffer(&(sg_buffer_desc){
		.data = SG_RANGE(dbg_vertices),
		.label = "debug-vertices"
	});
	state.dbg.pip = sg_make_pipeline(&(sg_pipeline_desc){
		.layout = {
			.attrs[ATTR_dbg_pos].format = SG_VERTEXFORMAT_FLOAT2,
		},
		.shader = sg_make_shader(dbg_shader_desc(sg_query_backend())),
		.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
		.label = "debug-pipeline",
	});

	// note: use a regular sampling-sampler to render the shadow map,
	// need to use nearest filtering here because of portability restrictions
	// (e.g. WebGL2)
	sg_sampler dbg_smp = sg_make_sampler(&(sg_sampler_desc){
		.min_filter = SG_FILTER_NEAREST,
		.mag_filter = SG_FILTER_NEAREST,
		.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
		.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
		.label = "debug-sampler"
	});
	state.dbg.bind = (sg_bindings){
		.vertex_buffers[0] = dbg_vbuf,
		.views[VIEW_dbg_tex] = state.display.shadow_map_tex_view,
		.samplers[SMP_dbg_smp] = dbg_smp,
	};
}

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
frame(
	void
) {
	uint64_t current_frame = stm_now();
	static uint64_t last_frame = 0;
	static uint64_t time_paused = 0;
	uint64_t diff = stm_diff(current_frame, last_frame);
	if (state.animation_paused) {
		time_paused += diff;
	}
	state.delta_time = stm_sec(diff);
	last_frame = current_frame;

	float rot = stm_sec(current_frame - time_paused) * 50.;

	HMM_Mat4 plane_model = HMM_Mul(HMM_Rotate_RH(HMM_AngleDeg(-90.f), X_AXIS),
	                               HMM_Scale(HMM_V3(10.f, 10.f, 10.f)));
	HMM_Vec3 tetrahedron_pos   = HMM_V3(0.0f, 1.5f, 0.0f);
	HMM_Mat4 tetrahedron_model = HMM_Translate(tetrahedron_pos);
	HMM_Vec3 plane_color       = HMM_V3(1.0f, 0.5f, 0.0f);
	HMM_Vec3 tetrahedron_color = HMM_V3(0.5f, 1.0f, 0.5f);

	// calculate matrices for shadow pass
	HMM_Mat4 light_rot  = HMM_Rotate_RH(HMM_AngleDeg(rot), Y_AXIS);
	HMM_Vec4 light_pos  = HMM_Mul(light_rot, HMM_V4(50.f, 50.f, -50.f, 1.0f));
	HMM_Mat4 light_view = HMM_LookAt_RH(light_pos.XYZ, tetrahedron_pos, CAMERA_UP);
	// NOTE(luke): use Z0 here and not NO variant of orthographic projection. */
	HMM_Mat4 light_proj = HMM_Orthographic_RH_ZO(-5.0f, 5.0f, -5.0f, 5.0f, ORTHOGRAPHIC_NEAR, FAR);
	HMM_Mat4 light_vp   = HMM_Mul(light_proj, light_view);

	vs_shadow_params_t tetrahedron_vs_shadow_params = {
		.mvp = HMM_Mul(light_vp, tetrahedron_model),
	};

	// calculate matrices for display pass
	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(state.camera.pitch, state.camera.yaw);
	HMM_Mat4 proj         = HMM_Perspective_RH_NO(HMM_AngleDeg(FOV_MAX),
	                                              sapp_widthf() / sapp_heightf(),
	                                              PERSPECTIVE_NEAR, FAR);
	HMM_Mat4 view         = HMM_LookAt_RH(state.camera.pos, HMM_Add(state.camera.pos, camera_front),
	                                      CAMERA_UP);
	HMM_Mat4 view_proj    = HMM_Mul(proj, view);

	fs_display_params_t fs_display_params = {
		.light_dir = HMM_NormV3(light_pos.XYZ),
		.eye_pos = state.camera.pos,
		.shadow_map_size = HMM_V2(SHADOW_MAP_SZ, SHADOW_MAP_SZ),
	};
	vs_display_params_t plane_vs_display_params = {
		.mvp = HMM_Mul(view_proj, plane_model),
		.model = plane_model,
		.normal_matrix = HMM_Transpose(HMM_InvGeneral(plane_model)),
		.light_mvp = HMM_Mul(light_vp, plane_model),
		.diff_color = plane_color,
	};
	vs_display_params_t tetrahedron_vs_display_params = {
		.mvp = HMM_Mul(view_proj, tetrahedron_model),
		.model = tetrahedron_model,
		.normal_matrix = HMM_Transpose(HMM_InvGeneral(tetrahedron_model)),
		.light_mvp = HMM_Mul(light_vp, tetrahedron_model),
		.diff_color = tetrahedron_color,
	};

	// the shadow map pass, render scene from light source into shadow map texture
	sg_begin_pass(&(sg_pass){
		// shadow map pass action: only clear depth buffer, don't configure color and stencil actions,
		// because there are no color and stencil targets
		.action.depth = {
			.load_action = SG_LOADACTION_CLEAR,
			.store_action = SG_STOREACTION_STORE,
			.clear_value = 1.0f,
		},
		// the pass only has a depth-stencil attachment
		.attachments.depth_stencil = state.shadow.depth_attachment,
	});
	sg_apply_pipeline(state.shadow.pip);
	sg_apply_uniforms(UB_vs_shadow_params, &SG_RANGE(tetrahedron_vs_shadow_params));
	mesh_draw_shadow(&state.tetrahedron_mesh);
	sg_end_pass();

	// the display pass, render scene from camera, compare-sample shadow map texture
	sg_begin_pass(&(sg_pass){
		.action = {
			.colors[0] = {
				.load_action = SG_LOADACTION_CLEAR,
				.clear_value = { 0.25f, 0.25f, 0.25f, 1.0f}
			},
		},
		.swapchain = sglue_swapchain(),
	});
	sg_apply_pipeline(state.display.pip);
	sg_apply_uniforms(UB_fs_display_params, &SG_RANGE(fs_display_params));

	sg_apply_uniforms(UB_vs_display_params, &SG_RANGE(tetrahedron_vs_display_params));
	mesh_draw_display(&state.tetrahedron_mesh, state.display.shadow_map_tex_view,
	                  state.display.shadow_sampler);

	sg_apply_uniforms(UB_vs_display_params, &SG_RANGE(plane_vs_display_params));
	mesh_draw_display(&state.plane_mesh, state.display.shadow_map_tex_view,
	                  state.display.shadow_sampler);

	// render debug visualization of shadow-map
	sg_apply_pipeline(state.dbg.pip);
	sg_apply_bindings(&state.dbg.bind);
	sg_apply_viewport(sapp_width() - DEBUG_SZ, 0, DEBUG_SZ, DEBUG_SZ, false);
	sg_draw(0, 4, 1);
	sg_end_pass();

	sg_commit();
}

static void
cleanup(
	void
) {
	sg_shutdown();
}

static void
app_handle_event(
	const sapp_event *event
) {
	if (!state.window_focused) {
		if (SAPP_EVENTTYPE_FOCUSED == event->type) {
			state.window_focused = true;
		}
		return;
	}
	if (SAPP_EVENTTYPE_UNFOCUSED == event->type) {
		state.window_focused = false;
	}
	if (   SAPP_EVENTTYPE_CHAR == event->type
	    && (   'q' == event->char_code
	        || 'Q' == event->char_code)) {
		sapp_quit();
	}
	f32 camera_speed = 10.f * state.delta_time;
	if (SAPP_EVENTTYPE_CHAR == event->type) {
		HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(state.camera.pitch, state.camera.yaw);
		if ('w' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(camera_front, camera_speed);
			state.camera.pos = HMM_Add(state.camera.pos, delta);
		}
		if ('s' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(camera_front, camera_speed);
			state.camera.pos = HMM_Sub(state.camera.pos, delta);
		}
		if ('a' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(HMM_Norm(HMM_Cross(camera_front, CAMERA_UP)), camera_speed);
			state.camera.pos = HMM_Sub(state.camera.pos, delta);
		}
		if ('d' == event->char_code) {
			HMM_Vec3 delta = HMM_Mul(HMM_Norm(HMM_Cross(camera_front, CAMERA_UP)), camera_speed);
			state.camera.pos = HMM_Add(state.camera.pos, delta);
		}
	}
	if (   SAPP_EVENTTYPE_KEY_DOWN == event->type
	    && !event->key_repeat) {
		if (SAPP_KEYCODE_SPACE == event->key_code) {
			state.animation_paused = !state.animation_paused;
		}
	}
	if (SAPP_EVENTTYPE_MOUSE_MOVE == event->type) {
		f32 sensitivity = 0.1f;
		f32 d_yaw   =  event->mouse_dx * sensitivity;
		f32 d_pitch = -event->mouse_dy * sensitivity; // positive `mouse_dy` is screen downward
		state.camera.yaw += d_yaw;
		state.camera.pitch = HMM_Clamp(-89.f, state.camera.pitch + d_pitch, 89.f);
	}
	if (SAPP_EVENTTYPE_MOUSE_SCROLL == event->type) {
		state.camera.fov = HMM_Clamp(1.f, state.camera.fov - event->scroll_y, FOV_MAX);
	}
}

sapp_desc
sokol_main(
	int argc,
	char* argv[]
) {
	(void)argc;
	(void)argv;
	return (sapp_desc){
		.init_cb = init,
		.frame_cb = frame,
		.cleanup_cb = cleanup,
		.event_cb = app_handle_event,
		.width = 1280,
		.height = 720,
		.sample_count = 4,
		.window_title = "shadows-depthtex-sapp.c",
		.icon.sokol_default = true,
		.logger.func = slog_func,
	};
}
