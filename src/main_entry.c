#include "grid.glsl.h"
#include "light.glsl.h"
#include "offscreen_passthru.glsl.h"
#include "outline.glsl.h"
#include "mesh.glsl.h"
#include "skybox.glsl.h"

#include "camera.h"
#include "image.h"
#include "mesh.h"
#include "render.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"

#include "handmade_math.h"
#include "texture.h"
#include "stb_image.h"
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"

#include <assert.h>
#include <float.h>
#include <stdlib.h>

#define GRID          0
#define NUM_INSTANCES 5
#define HALF_GRID 1000.f

#define SHADOW_MAP_SZ 1024
#define DEBUG_SZ       200

typedef HMM_Vec3 SkyboxVertex;
static const SkyboxVertex skybox_vertices[] = {
	{ .X = -1.f,  1.f, -1.f },
	{ .X = -1.f, -1.f, -1.f },
	{ .X =  1.f, -1.f, -1.f },
	{ .X =  1.f, -1.f, -1.f },
	{ .X =  1.f,  1.f, -1.f },
	{ .X = -1.f,  1.f, -1.f },
	{ .X = -1.f, -1.f,  1.f },
	{ .X = -1.f, -1.f, -1.f },
	{ .X = -1.f,  1.f, -1.f },
	{ .X = -1.f,  1.f, -1.f },
	{ .X = -1.f,  1.f,  1.f },
	{ .X = -1.f, -1.f,  1.f },
	{ .X =  1.f, -1.f, -1.f },
	{ .X =  1.f, -1.f,  1.f },
	{ .X =  1.f,  1.f,  1.f },
	{ .X =  1.f,  1.f,  1.f },
	{ .X =  1.f,  1.f, -1.f },
	{ .X =  1.f, -1.f, -1.f },
	{ .X = -1.f, -1.f,  1.f },
	{ .X = -1.f,  1.f,  1.f },
	{ .X =  1.f,  1.f,  1.f },
	{ .X =  1.f,  1.f,  1.f },
	{ .X =  1.f, -1.f,  1.f },
	{ .X = -1.f, -1.f,  1.f },
	{ .X = -1.f,  1.f, -1.f },
	{ .X =  1.f,  1.f, -1.f },
	{ .X =  1.f,  1.f,  1.f },
	{ .X =  1.f,  1.f,  1.f },
	{ .X = -1.f,  1.f,  1.f },
	{ .X = -1.f,  1.f, -1.f },
	{ .X = -1.f, -1.f, -1.f },
	{ .X = -1.f, -1.f,  1.f },
	{ .X =  1.f, -1.f, -1.f },
	{ .X =  1.f, -1.f, -1.f },
	{ .X = -1.f, -1.f,  1.f },
	{ .X =  1.f, -1.f,  1.f },
};

typedef struct {
	sg_buffer vertex_buffer;
	sg_sampler sampler; // to sample into skybox
	sg_view texture;
	sg_image texture_image;
} Skybox;

typedef struct {
	Mesh backpack_mesh;
	MeshInstances backpack_instances;
	Mesh light_cube_mesh;
	Mesh horiz_grid_mesh;
	Lighting lighting;
	sg_pipeline display_pipeline;
	sg_pipeline light_cube_pipeline;
	sg_pipeline horiz_grid_pipeline;
	sg_pipeline fullscreen_quad_pipeline;
	sg_bindings fullscreen_quad_bindings;
	sg_pipeline skybox_pipeline;
	sg_view color_attachment;
	sg_view resolve_attachment_msaa;
	sg_view depth_stencil_attachment;
	Skybox skybox;
	Camera camera;
	struct {
		sg_view depth_attachment;
		sg_image depth_img;
		sg_pipeline pip;
	} shadow;
	struct {
		sg_pipeline pip;
		sg_bindings bind;
	} dbg;
	sg_view shadow_map_tex_view;
	sg_sampler shadow_map_sampler;
	f32 delta_time;
	bool window_focused;
	bool animation_paused;
} AppState;

static const DirLight_t g_dir_lights[] = {
	{
		.direction = { .X =  0.5f , -0.7f  , -0.2f  },
		.ambient   = { .R =  0.01f,  0.01f,  0.01f },
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

static const PointLight_t g_point_lights[] = {
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

static sg_bindings
light_cube_bindings(
	Mesh *mesh
) {
	return (sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.index_buffer = mesh->index_buffer,
	};
}

static sg_bindings
horiz_grid_bindings(
	Mesh *mesh
) {
	return (sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.index_buffer = mesh->index_buffer,
	};
}

static sg_bindings
skybox_bindings(
	Skybox *skybox
) {
	return (sg_bindings){
		.vertex_buffers[0] = skybox->vertex_buffer,
		.samplers[SMP_skyboxSampler] = skybox->sampler,
		.views[VIEW_skyboxTexture] = skybox->texture,
	};
}

static sg_bindings
backpack_bindings(
	Mesh *mesh,
	Lighting *lighting,
	sg_buffer instance_buffer,
	sg_sampler shadow_sampler,
	sg_view shadow_map_tex_view,
	sg_sampler skybox_sampler,
	sg_view skybox_texture
) {
	(void)skybox_sampler;
	(void)skybox_texture;
	sg_bindings bindings = (sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.vertex_buffers[1] = instance_buffer,
		.index_buffer = mesh->index_buffer,
		.samplers[SMP_texSampler] = mesh_display_sampler(),
		.views[VIEW_in_dir_lights] = lighting->directional.view,
		.views[VIEW_in_point_lights] = lighting->point.view,
		.views[VIEW_shadow_map] = shadow_map_tex_view,
		.samplers[SMP_shadow_sampler] = shadow_sampler,
		// /* Environment mapping */
		// .samplers[SMP_envSampler] = skybox_sampler,
		// .views[VIEW_envTexture] = skybox_texture,
	};
	mesh_apply_textures_to_bindings(mesh, &bindings);
	return bindings;
}

static void
load_obj_as_mesh(
	Mesh *mesh,
	const char *filename
) {
	fastObjMesh *read = fast_obj_read(filename);

	assert(read->position_count < MESH_INDEX_MAX);
	MeshVertexView vertices = {
		.ptr = malloc(sizeof(*vertices.ptr) * read->position_count),
		.len = read->position_count,
	};
	MeshTriangleView triangles = {
		.ptr = malloc(sizeof(*triangles.ptr) * read->face_count),
		.len = read->face_count,
	};
	MeshIndex *write_to_index = &triangles.ptr[0].indices[0];
	for (usize i = 0; i < triangles.len * 3; ++i) {
		fastObjIndex *index = read->indices + i;
		write_to_index[i] = (MeshIndex)index->p;
		vertices.ptr[index->p] = (MeshVertex){
			.position  = *(HMM_Vec3 *)(read->positions + index->p * 3),
			.normal    = *(HMM_Vec3 *)(read->normals   + index->n * 3),
			.tex_coord = *(HMM_Vec2 *)(read->texcoords + index->t * 2),
		};
	}
	mesh_fixup_bivectors(&vertices, &triangles, false);
	mesh_init_raw(&vertices, &triangles, mesh);

	fast_obj_destroy(read);
	free(vertices.ptr);
	free(triangles.ptr);
}

static void
init_lighting(
	Lighting *lighting
) {
	static bool done_once = false;
	assert(!done_once);
	done_once = true;
	(void)done_once;

	sg_buffer dir_buffer = sg_make_buffer(&(sg_buffer_desc){
		.usage.storage_buffer = true,
		.usage.immutable = true,
		.data = SG_RANGE(g_dir_lights),
		.label = "directional-lights",
	});
	sg_buffer point_buffer = sg_make_buffer(&(sg_buffer_desc){
		.usage.storage_buffer = true,
		.usage.immutable = true,
		.data = SG_RANGE(g_point_lights),
		.label = "point-lights",
	});

	*lighting = (Lighting){
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

static sg_image
init_skybox_texture_image(
	void
) {
	stbi_set_flip_vertically_on_load(false);
	int w_expected = 2048, h_expected = 2048;
	usize surface_pitch = w_expected * h_expected * 4;
	usize cube_texture_sz = surface_pitch * 6;
	assert(surface_pitch == (usize)sg_query_surface_pitch(SG_PIXELFORMAT_RGBA8, w_expected, h_expected, 1));
	byte *bytes = malloc(cube_texture_sz);
	const char *paths[] = { "right", "left", "top", "bottom", "front", "back" };
	char path_buf[32];
	for (usize i = 0; i < countof(paths); ++i) {
		int w, h, n_channels;
		snprintf(path_buf, sizeof(path_buf), "assets/skybox/%s.jpg", paths[i]);
		byte *data = stbi_load(path_buf, &w, &h, &n_channels, 4);
		assert(data);
		assert(h == h_expected && w == w_expected);
		memcpy(bytes + i * surface_pitch, data, surface_pitch);
		stbi_image_free(data);
	}
	sg_image img = sg_make_image(&(sg_image_desc){
		.type = SG_IMAGETYPE_CUBE,
		.width = w_expected,
		.height = h_expected,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data.mip_levels[0] = { .ptr = bytes, .size = cube_texture_sz, },
	});
	free(bytes);
	return img;
}

static void
init_skybox(
	Skybox *skybox
) {
	sg_image skybox_img = init_skybox_texture_image();
	*skybox = (Skybox){
		.vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.usage.immutable = true,
			.data = SG_RANGE(skybox_vertices),
			.label = "skybox-vertex-buffer",
		}),
		.sampler = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
			.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
			.wrap_w = SG_WRAP_CLAMP_TO_EDGE,
			.label = "skybox-samplers"
		}),
		.texture = sg_make_view(&(sg_view_desc){
			.texture = {
				.image = skybox_img,
			},
			.label = "skybox-texture",
		}),
		.texture_image = skybox_img,
	};
}

static void
init_light_cube_mesh(
	Mesh *mesh
) {
	mesh_gen_primitive_cube(mesh);
}

static void
init_horiz_grid_mesh(
	Mesh *mesh
) {
	mesh_gen_primitive_plane(mesh);
}

static void
init_backpack_mesh(
	Mesh *mesh,
	MeshInstances *instances
) {
	load_obj_as_mesh(mesh, "assets/backpack.obj");

	sg_image diffuse_img, specular_img;
	load_image("assets/backpack_diffuse.jpg", false, &diffuse_img);
	load_image("assets/backpack_specular.jpg", false, &specular_img);

	mesh_set_texture(mesh, diffuse_img, TextureKindDiffuse);
	mesh_set_texture(mesh, specular_img, TextureKindSpecular);
	*instances = mesh_instances(NUM_INSTANCES);
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

static HMM_Mat4
projection_matrix(
	Camera *camera
) {
	f32 aspect_ratio = sapp_widthf() / sapp_heightf();
	return HMM_Perspective_RH_NO(HMM_AngleDeg(camera->fov), aspect_ratio, PERSPECTIVE_NEAR, 1000.f);
}

void
compute_display_params(
	f32 rotation,
	Camera *camera,
	vs_display_params_t *params
) {
	params->model = HMM_Mul(HMM_Translate(HMM_V3(0.f, 0.f, 0.f)),
	                        HMM_Rotate_RH(HMM_AngleDeg(-rotation), HMM_V3(0.f, 1.0f, 0.0f)));
	params->normal_matrix = HMM_Transpose(HMM_InvGeneral(params->model));
	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(camera->pitch, camera->yaw);
	params->vp = HMM_Mul(projection_matrix(camera),
	                     HMM_LookAt_RH(camera->pos, HMM_Add(camera->pos, camera_front), CAMERA_UP));

	HMM_Vec3 equiv_light_pos = HMM_Mul(g_dir_lights[0].direction, -10.f);
	HMM_Mat4 light_view = HMM_LookAt_RH(equiv_light_pos, ORIGIN, CAMERA_UP);
	HMM_Mat4 light_proj = HMM_Orthographic_RH_ZO(-10.0f, 10.0f, -10.0f, 10.0f, ORTHOGRAPHIC_NEAR, FAR);
	params->light_vp = HMM_Mul(light_proj, light_view);
}

static void
draw_mesh_shadows(
	sg_bindings *bindings,
	Mesh *mesh,
	usize num_instances,
	vs_display_params_t *display_params
) {
	sg_apply_bindings(bindings);
	vs_shadow_params_t shadow_params = {
		.vp = display_params->light_vp,
		.model = display_params->model,
	};
	sg_apply_uniforms(UB_vs_shadow_params, &SG_RANGE(shadow_params));
	sg_draw(0, (int)mesh->num_elements, num_instances);
}

static void
draw_backpack_mesh(
	sg_bindings *bindings,
	Mesh *mesh,
	usize num_instances,
	Camera *camera,
	Lighting *lighting,
	vs_display_params_t *display_params
) {
	sg_apply_bindings(bindings);
	sg_apply_uniforms(UB_vs_display_params, &SG_RANGE(*display_params));
	fs_display_params_t scene_fs_params = {
		.light_dir = g_dir_lights[0].direction,
		.eye_pos = camera->pos,
		.n_dir_lights = lighting->directional.len,
		// .n_point_lights = lighting->point.len,
		.n_point_lights = 0,
	};
	sg_apply_uniforms(UB_fs_display_params, &SG_RANGE(scene_fs_params));
	sg_draw(0, mesh->num_elements, num_instances);
}

static void
draw_skybox(
	sg_bindings *bindings,
	Camera *camera
) {
	sg_apply_bindings(bindings);

	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(camera->pitch, camera->yaw);
	HMM_Mat4 view = HMM_LookAt_RH(camera->pos, HMM_Add(camera->pos, camera_front), CAMERA_UP);
	/* Strip translation from view matrix; keep rotation. */
	view.Columns[3] = (HMM_Vec4){ .W = 1.f};
	skybox_vs_params_t skybox_vs_params = {
		.view       = view,
		.projection = projection_matrix(camera),
	};
	sg_apply_uniforms(UB_skybox_vs_params, &SG_RANGE(skybox_vs_params));

	sg_draw(0, countof(skybox_vertices), 1);
}

static void
draw_light_cubes(
	sg_bindings *bindings,
	Mesh *mesh,
	Camera *camera
) {
	sg_apply_bindings(bindings);

	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(camera->pitch, camera->yaw);
	light_vs_params_t light_vs_params = {
		.view       = HMM_LookAt_RH(camera->pos, HMM_Add(camera->pos, camera_front), CAMERA_UP),
		.projection = projection_matrix(camera),
	};
	for (usize i = 0; i < countof(g_point_lights); ++i){
		light_vs_params.model = HMM_Mul(HMM_Translate(g_point_lights[i].position),
		                        HMM_Scale(HMM_V3(0.2f, 0.2f, 0.2f)));
		sg_apply_uniforms(UB_light_vs_params, &SG_RANGE(light_vs_params));
		light_fs_params_t light_fs_params = {
			.light_color = g_point_lights[i].specular,
		};
		sg_apply_uniforms(UB_light_fs_params, &SG_RANGE(light_fs_params));
		sg_draw(0, mesh->num_elements, 1);
	}
}

static void
draw_horiz_grid(
	sg_bindings *bindings,
	Mesh *mesh,
	Camera *camera
) {
#if GRID
#else
	return;
#endif
	sg_apply_bindings(bindings);

	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(camera->pitch, camera->yaw);
	/* Keep the grid centered on the camera X-Z, so it feels "infinite". Snap to some fixed interval
	   to avoid artifacts while panning. */
	float interval = 10.f;
	HMM_Vec3 grid_pos = {
		.X = floorf(camera->pos.X / interval) * interval,
		.Z = floorf(camera->pos.Z / interval) * interval,
	};
	grid_vs_params_t grid_vs_params = {
		.view       = HMM_LookAt_RH(camera->pos, HMM_Add(camera->pos, camera_front), CAMERA_UP),
		.projection = projection_matrix(camera),
		.model      = HMM_Mul(HMM_Translate(grid_pos),
		                      HMM_Mul(HMM_Rotate_RH(HMM_AngleDeg(-90.f), X_AXIS),
		                              HMM_Scale(HMM_V3(HALF_GRID, HALF_GRID, 1.f)))),
	};

	sg_apply_uniforms(UB_grid_vs_params, &SG_RANGE(grid_vs_params));
	grid_fs_params_t grid_fs_params = {
		.grid_color = HMM_V3(0.5f, 0.5f, 0.5f),
	};
	sg_apply_uniforms(UB_grid_fs_params, &SG_RANGE(grid_fs_params));
	sg_draw(0, mesh->num_elements, 1);
}

static void
state_init(
	AppState *state
) {
	// how to handle resizing the window??
	sg_image color_attachment_img = sg_make_image(&(sg_image_desc){
		.usage.color_attachment = true,
		.usage.immutable = true,
		.width = sapp_width(),
		.height = sapp_height(),
		.sample_count = OFFSCREEN_SAMPLE_COUNT,
	});
	sg_image resolve_img = sg_make_image(&(sg_image_desc){
		.usage.resolve_attachment = true,
		.usage.immutable = true,
		.width = sapp_width(),
		.height = sapp_height(),
		.sample_count = 1,
	});
	sg_image depth_stencil_attachment_img = sg_make_image(&(sg_image_desc){
		.usage.depth_stencil_attachment = true,
		.usage.immutable = true,
		.width = sapp_width(),
		.height = sapp_height(),
		.sample_count = OFFSCREEN_SAMPLE_COUNT,
	});
	sg_image shadow_depth_img = sg_make_image(&(sg_image_desc){
		.usage.depth_stencil_attachment = true,
		.width = SHADOW_MAP_SZ,
		.height = SHADOW_MAP_SZ,
		.pixel_format = SG_PIXELFORMAT_DEPTH,
		.sample_count = SHADOW_SAMPLE_COUNT,
		.label = "shadow-map",
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
	float dbg_vertices[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
	sg_buffer dbg_vbuf = sg_make_buffer(&(sg_buffer_desc){
		.data = SG_RANGE(dbg_vertices),
		.label = "debug-vertices"
	});
	sg_view shadow_map_tex_view = sg_make_view(&(sg_view_desc){
		.texture = { .image = shadow_depth_img },
		.label = "shadow-map-tex-view",
	});

	*state = (AppState){
		.light_cube_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(light_shader_desc(sg_query_backend())),
			.layout = (sg_vertex_layout_state){
				.attrs = {
					[ATTR_light_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				},
				.buffers = {
					[ATTR_light_aPos].stride = sizeof(MeshVertex),
				},
			},
			.depth = (sg_depth_state){
				.write_enabled = true,
				.compare = SG_COMPAREFUNC_LESS,
			},
			.index_type = SG_INDEXTYPE_UINT16, // should match `INDEX_TYPE`
			.label = "light-cube-pipeline",
		}),
		.horiz_grid_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(grid_shader_desc(sg_query_backend())),
			.layout = (sg_vertex_layout_state){
				.attrs = {
					[ATTR_grid_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				},
				.buffers = {
					[ATTR_grid_aPos].stride = sizeof(MeshVertex),
				},
			},
			.depth = (sg_depth_state){
				.write_enabled = true,
				.compare = SG_COMPAREFUNC_LESS,
			},
			.color_count = 1,
			.colors[0] = {
				.blend = {
					.enabled = true,
					.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
					.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.op_rgb = SG_BLENDOP_ADD,
					.src_factor_alpha = SG_BLENDFACTOR_ONE,
					.dst_factor_alpha = SG_BLENDFACTOR_ZERO,
					.op_alpha = SG_BLENDOP_ADD,
				},
			},
			.index_type = SG_INDEXTYPE_UINT16, // should match `INDEX_TYPE`
			.label = "grid-pipeline",
		}),
		.fullscreen_quad_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(offscreen_passthru_shader_desc(sg_query_backend())),
			.label = "fullscreen-quad-pipeline",
		}),
		.fullscreen_quad_bindings = (sg_bindings){
			.samplers[SMP_texSampler] = sg_make_sampler(&(sg_sampler_desc){
				.min_filter = SG_FILTER_NEAREST,
				.mag_filter = SG_FILTER_NEAREST,
				.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
				.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
				.label = "fullscreen_quad-sampler"
			}),
			.views[VIEW_offscreenTexture] = sg_make_view(&(sg_view_desc){
				.texture.image = resolve_img,
				.label = "offscreen-texture-view",
			}),
		},
		.skybox_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(skybox_shader_desc(sg_query_backend())),
			.layout = (sg_vertex_layout_state){
				.attrs = {
					[ATTR_skybox_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				},
				.buffers[0] = { .stride = sizeof(SkyboxVertex) },
			},
			.depth = (sg_depth_state){
				.write_enabled = false,
				.compare = SG_COMPAREFUNC_LESS_EQUAL,
			},
			.label = "skybox-pipeline",
		}),
		.shadow = {
			.depth_attachment = sg_make_view(&(sg_view_desc){
				.depth_stencil_attachment = { .image = shadow_depth_img },
				.label = "shadow-map-depth-stencil-view",
			}),
			.depth_img = shadow_depth_img,
		},
		.shadow_map_tex_view = shadow_map_tex_view,
		.shadow_map_sampler = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.wrap_u = SG_WRAP_CLAMP_TO_BORDER,
			.wrap_v = SG_WRAP_CLAMP_TO_BORDER,
			.border_color = SG_BORDERCOLOR_OPAQUE_BLACK,
			.compare = SG_COMPAREFUNC_LESS,
			.label = "shadow-sampler",
		}),
		.color_attachment = sg_make_view(&(sg_view_desc){
			.color_attachment.image = color_attachment_img,
			.label = "color-attachment-view",
		}),
		.resolve_attachment_msaa = sg_make_view(&(sg_view_desc){
			.resolve_attachment.image = resolve_img,
			.label = "resolve-attachment-msaa",
		}),
		.depth_stencil_attachment = sg_make_view(&(sg_view_desc){
			.depth_stencil_attachment.image = depth_stencil_attachment_img,
			.label = "depth-stencil-attachment-view",
		}),
		.dbg.bind = (sg_bindings){
			.vertex_buffers[0] = dbg_vbuf,
			.views[VIEW_dbg_tex] = shadow_map_tex_view,
			.samplers[SMP_dbg_smp] = dbg_smp,
		},
		.dbg.pip = sg_make_pipeline(&(sg_pipeline_desc){
			.layout = {
				.attrs[ATTR_dbg_pos].format = SG_VERTEXFORMAT_FLOAT2,
			},
			.shader = sg_make_shader(dbg_shader_desc(sg_query_backend())),
			.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
			.label = "debug-pipeline",
		}),
		.camera = (Camera){
			.fov = FOV_MAX,
			.pitch = -10.f,
			.yaw = -90.f,
			.pos = HMM_V3(0.f, 2.0f,  10.f),
		},
		.window_focused = true,
		.animation_paused = false,
	};

	init_lighting(&state->lighting);
	init_backpack_mesh(&state->backpack_mesh, &state->backpack_instances);
	init_skybox(&state->skybox);
	init_light_cube_mesh(&state->light_cube_mesh);
	init_horiz_grid_mesh(&state->horiz_grid_mesh);

	init_mesh_shadow_pipeline(&state->shadow.pip);
	init_mesh_display_pipeline(&state->display_pipeline);
}

sg_bindings
shadow_bindings(
	Mesh *mesh,
	sg_buffer instance_buffer
) {
	return (sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.vertex_buffers[1] = instance_buffer,
		.index_buffer = mesh->index_buffer,
	};
}

static void
app_frame(
	void *state_
) {
	AppState *state = state_;

	uint64_t current_frame = stm_now();
	static uint64_t last_frame = 0;
	static uint64_t time_paused = 0;
	uint64_t diff = stm_diff(current_frame, last_frame);
	if (state->animation_paused) {
		time_paused += diff;
	}
	state->delta_time = stm_sec(diff);
	last_frame = current_frame;

	float backpack_rotation = stm_sec(current_frame - time_paused) * 10.;
	HMM_Vec3 per_instance_positions[NUM_INSTANCES];
	for (usize i = 0; i < countof(per_instance_positions); ++i) {
		per_instance_positions[i] = HMM_V3(-8.f + 4.f * i, 0., 0.);
	}
	sg_update_buffer(state->backpack_instances.buffer, &SG_RANGE(per_instance_positions));

	vs_display_params_t display_params;
	compute_display_params(backpack_rotation, &state->camera, &display_params);

	sg_bindings bindings;

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
		.attachments.depth_stencil = state->shadow.depth_attachment,
	});
	sg_apply_pipeline(state->shadow.pip);
	bindings = shadow_bindings(&state->backpack_mesh, state->backpack_instances.buffer);
	draw_mesh_shadows(&bindings, &state->backpack_mesh, state->backpack_instances.num_instances,
	                  &display_params);
	sg_end_pass();
	
	sg_begin_pass(&(sg_pass){
		.action = (sg_pass_action) {
			.colors[0] = {
				.load_action = SG_LOADACTION_CLEAR,
				.store_action = SG_STOREACTION_DONTCARE, // b/c it's only needed until the msaa resolve
				.clear_value = { 0.1f, 0.1f, 0.1f, 1.0f },
			},
			.depth = {
				.load_action = SG_LOADACTION_CLEAR,
				.store_action = SG_STOREACTION_STORE,
				.clear_value = 1.f,
			},
			.stencil = {
				.load_action = SG_LOADACTION_CLEAR,
				.store_action = SG_STOREACTION_DONTCARE,
				.clear_value = 0,
			},
		},
		.attachments = {
			.colors[0] = state->color_attachment,
			.resolves[0] = state->resolve_attachment_msaa,
			.depth_stencil = state->depth_stencil_attachment,
		},
	});

	sg_apply_pipeline(state->display_pipeline);
	bindings = backpack_bindings(&state->backpack_mesh, &state->lighting,
	                             state->backpack_instances.buffer,
	                             state->shadow_map_sampler, state->shadow_map_tex_view,
	                             state->skybox.sampler, state->skybox.texture);
	draw_backpack_mesh(&bindings, &state->backpack_mesh, state->backpack_instances.num_instances,
	                   &state->camera, &state->lighting, &display_params);

	sg_apply_pipeline(state->light_cube_pipeline);
	bindings = light_cube_bindings(&state->light_cube_mesh);
	draw_light_cubes(&bindings, &state->light_cube_mesh, &state->camera);

	sg_apply_pipeline(state->skybox_pipeline);
	bindings = skybox_bindings(&state->skybox);
	draw_skybox(&bindings, &state->camera);

	sg_apply_pipeline(state->horiz_grid_pipeline);
	bindings = horiz_grid_bindings(&state->horiz_grid_mesh);
	draw_horiz_grid(&bindings, &state->horiz_grid_mesh, &state->camera);

	sg_end_pass();

	sg_begin_pass(&(sg_pass){
		.action = (sg_pass_action) {
			.colors[0] = {
				.load_action = SG_LOADACTION_CLEAR,
				.store_action = SG_STOREACTION_STORE,
				.clear_value = { 0.25f, 0.65f, 0.45f, 1.f },
			},
		},
		.swapchain = sglue_swapchain(),
	});

	sg_apply_pipeline(state->fullscreen_quad_pipeline);
	sg_apply_bindings(&state->fullscreen_quad_bindings);
	postprocess_params_t postprocess = {
		.screen_dims = HMM_V2(sapp_width(), sapp_height()),
	};
	sg_apply_uniforms(UB_postprocess_params, &SG_RANGE(postprocess));

	sg_draw(0, 3, 1);

	sg_apply_pipeline(state->dbg.pip);
	sg_apply_bindings(&state->dbg.bind);
	sg_apply_viewport(sapp_width() - DEBUG_SZ, 0, DEBUG_SZ, DEBUG_SZ, false);
	sg_draw(0, 4, 1);

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
	if (   SAPP_EVENTTYPE_KEY_DOWN == event->type
	    && !event->key_repeat) {
		if (SAPP_KEYCODE_SPACE == event->key_code) {
			state->animation_paused = !state->animation_paused;
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
	sg_features features = sg_query_features();
	if (!features.origin_top_left) {
		assert(false); // post-processing shader and any kernels therein needs to be flipped about y-axis
	}
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
		.sample_count = DISPLAY_SAMPLE_COUNT,
		.width = 1280,
		.height = 720,
		.window_title = "Triangle",
		.icon.sokol_default = true,
		.logger.func = slog_func,
	};
}
