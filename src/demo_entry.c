#include "camera.h"
#include "image.h"
#include "mesh.glsl.h"
#include "mesh.h"
#include "primitives.h"
#include "render.h"

#include "handmade_math.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"
#include "texture.h"

#include <stdio.h>
#include <assert.h>

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
	Lighting lighting;
	bool animation_paused;
	bool window_focused;
	f32 delta_time;
} state = {0};

static DirLight_t g_dir_lights[] = {
	{
		.direction = { .X = -0.2f , -1.f  , -0.3f  },
		.ambient   = { .R =  0.25f,  0.25f,  0.25f },
		.diffuse   = { .R =  0.4f ,  0.4f ,  0.4f  },
		.specular  = { .R =  0.5f ,  0.5f ,  0.5f  },
	},
};
static const PointLight_t g_point_lights[] = {
	{
		.position  = { .X =  0.7f ,  0.2f ,  2.0f },
		.ambient   = { .R =  0.05f,  0.05f,  0.05f },
		.diffuse   = { .R =  0.8f ,  0.8f ,  0.8f  },
		.specular  = { .R =  1.0f ,  1.0f ,  1.0f  },
		.constant  = 1.f,
		.linear    = 0.09f,
		.quadratic = 0.032f,
	},
};

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
	mesh_set_texture_defaults(&state.tetrahedron_mesh);
	mesh_set_texture_defaults(&state.plane_mesh);

	sg_image diffuse_img;
	load_image("assets/awesomeface.png", true, &diffuse_img);
	mesh_set_texture(&state.plane_mesh, diffuse_img, TextureKindDiffuse);

	init_mesh_shadow_pipeline(&state.shadow.pip);
	init_mesh_display_pipeline(&state.display.pip);

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
	sg_buffer dir_buffer = sg_make_buffer(&(sg_buffer_desc){
		.usage.storage_buffer = true,
		.usage.stream_update = true,
		.size = sizeof(g_dir_lights),
		.label = "directional-lights",
	});
	sg_buffer point_buffer = sg_make_buffer(&(sg_buffer_desc){
		.usage.storage_buffer = true,
		.usage.immutable = true,
		.data = SG_RANGE(g_point_lights),
		.label = "point-lights",
	});
	state.lighting = (Lighting){
		.directional = {
			.view = sg_make_view(&(sg_view_desc){
				.storage_buffer = { .buffer = dir_buffer },
				.label = "directional-lights-storage",
			}),
			.len = countof(g_dir_lights),
			.buffer = dir_buffer,
		},
		.point = {
			.view = sg_make_view(&(sg_view_desc){
				.storage_buffer = { .buffer = point_buffer },
				.label = "point-lights-storage",
			}),
			.len = countof(g_point_lights),
			.buffer = point_buffer,
		},
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

void
mesh_draw_shadow(
	Mesh *mesh
) {
	sg_bindings bindings = (sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.vertex_buffers[1] = mesh_one_instance_buffer(),
		.index_buffer = mesh->index_buffer,
	};
	sg_apply_bindings(&bindings);
	sg_draw(0, (int)mesh->num_elements, 1);
}

static void
mesh_draw_display(
	Mesh *mesh,
	sg_view shadow_map_tex_view,
	sg_sampler shadow_sampler,
	Lighting *lighting
){
	sg_bindings bindings = (sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.vertex_buffers[1] = mesh_one_instance_buffer(),
		.index_buffer = mesh->index_buffer,
		.views[VIEW_shadow_map] = shadow_map_tex_view,
		.samplers[SMP_shadow_sampler] = shadow_sampler,
		.samplers[SMP_tex_sampler] = mesh_display_sampler(),
		.views[VIEW_in_dir_lights] = lighting->directional.view,
		.views[VIEW_in_point_lights] = lighting->point.view,
	};
	mesh_apply_textures_to_bindings(mesh, &bindings);
	sg_apply_bindings(&bindings);
	sg_draw(0, (int)mesh->num_elements, 1);
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
	state.delta_time = (f32)stm_sec(diff);
	last_frame = current_frame;

	f32 rot = (f32)stm_sec(current_frame - time_paused) * 50.f;

	HMM_Mat4 plane_model = HMM_Mul(HMM_Rotate_RH(HMM_AngleDeg(-90.f), X_AXIS),
	                               HMM_Scale(HMM_V3(10.f, 10.f, 10.f)));
	HMM_Vec3 tetrahedron_pos   = HMM_V3(0.0f, 1.5f, 0.0f);
	HMM_Mat4 tetrahedron_model = HMM_Translate(tetrahedron_pos);

	// calculate matrices for shadow pass
	HMM_Mat4 light_rot  = HMM_Rotate_RH(HMM_AngleDeg(rot), Y_AXIS);
	HMM_Vec3 light_pos  = HMM_Mul(light_rot, HMM_V4(50.f, 50.f, -50.f, 1.0f)).XYZ;
	HMM_Vec3 light_dir  = HMM_Norm(HMM_Sub(tetrahedron_pos, light_pos));
	HMM_Mat4 light_view = HMM_LookAt_RH(light_pos, tetrahedron_pos, CAMERA_UP);
	// NOTE(luke): use Z0 here and not NO variant of orthographic projection. */
	HMM_Mat4 light_proj = HMM_Orthographic_RH_ZO(-5.0f, 5.0f, -5.0f, 5.0f, ORTHOGRAPHIC_NEAR, FAR);
	HMM_Mat4 light_vp   = HMM_Mul(light_proj, light_view);

	vs_shadow_params_t tetrahedron_vs_shadow_params = {
		.vp = light_vp,
		.model = tetrahedron_model,
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
		.light_dir = light_dir,
		.eye_pos = state.camera.pos,
		.n_dir_lights = state.lighting.directional.len,
		// .n_point_lights = state.lighting.point.len,
		.n_point_lights = 0,
	};
	vs_display_params_t plane_vs_display_params = {
		.vp = view_proj,
		.model = plane_model,
		.normal_matrix = HMM_Transpose(HMM_InvGeneral(plane_model)),
		.light_vp = light_vp,
	};
	vs_display_params_t tetrahedron_vs_display_params = {
		.vp = view_proj,
		.model = tetrahedron_model,
		.normal_matrix = HMM_Transpose(HMM_InvGeneral(tetrahedron_model)),
		.light_vp = light_vp,
	};

	assert(1 == state.lighting.directional.len);
	assert(1 == countof(g_dir_lights));
	g_dir_lights[0].direction = light_dir;
	sg_update_buffer(state.lighting.directional.buffer, &SG_RANGE(g_dir_lights));

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
	                  state.display.shadow_sampler, &state.lighting);

	sg_apply_uniforms(UB_vs_display_params, &SG_RANGE(plane_vs_display_params));
	mesh_draw_display(&state.plane_mesh, state.display.shadow_map_tex_view,
	                  state.display.shadow_sampler, &state.lighting);

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
