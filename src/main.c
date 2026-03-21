#include "grid.glsl.h"
#include "light.glsl.h"
#include "offscreen_passthru.glsl.h"
#include "outline.glsl.h"
#include "scene.glsl.h"
#include "skybox.glsl.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "sokol_time.h"

#include "handmade_math.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"

#include <float.h>
#include <stdlib.h>

typedef float f32;
typedef uint16_t u16;
typedef uint32_t u32;
typedef size_t usize;
typedef unsigned char byte;

typedef u16 INDEX_TYPE;

#define countof(x) (sizeof(x) / sizeof((x)[0]))

#define WIDTH 1280
#define HEIGHT 720
#define FOV_MAX 45.f

#define MESH_OUTLINE 0
#define GRID         1

#if 0
#define W_CHECKERBOARD 4
#define H_CHECKERBOARD 4
static const byte checkerboard_data[4 * W_CHECKERBOARD * H_CHECKERBOARD] = {
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 
};

static
void checkerboard(
	sg_image *img
) {
	*img = sg_make_image(&(sg_image_desc){
		.width = W_CHECKERBOARD,
		.height = H_CHECKERBOARD,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data.mip_levels[0] = {
			.ptr = checkerboard_data,
			.size = sizeof(checkerboard_data),
		},
	});
}

static const byte solid_white_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

static
void solid_white(
	sg_image *img
) {
	*img = sg_make_image(&(sg_image_desc){
		.width = 1,
		.height = 1,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.data.mip_levels[0] = {
			.ptr = solid_white_data,
			.size = sizeof(solid_white_data),
		},
	});
}
#endif

typedef struct {
	HMM_Vec3 pos;
	HMM_Vec3 norm;
	HMM_Vec2 tex;
} Vertex;

static const Vertex cube_vertices[] = {
	{ { .X = -0.5f, -0.5f, -0.5f }, { .X =  0.0f,  0.0f, -1.0f}, { .S = 0.0f, 0.0f } },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .X =  0.0f,  0.0f, -1.0f}, { .S = 1.0f, 0.0f } },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .X =  0.0f,  0.0f, -1.0f}, { .S = 1.0f, 1.0f } },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .X =  0.0f,  0.0f, -1.0f}, { .S = 1.0f, 1.0f } },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .X =  0.0f,  0.0f, -1.0f}, { .S = 0.0f, 1.0f } },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .X =  0.0f,  0.0f, -1.0f}, { .S = 0.0f, 0.0f } },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .X =  0.0f,  0.0f,  1.0f}, { .S = 0.0f, 0.0f } },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .X =  0.0f,  0.0f,  1.0f}, { .S = 1.0f, 0.0f } },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .X =  0.0f,  0.0f,  1.0f}, { .S = 1.0f, 1.0f } },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .X =  0.0f,  0.0f,  1.0f}, { .S = 1.0f, 1.0f } },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .X =  0.0f,  0.0f,  1.0f}, { .S = 0.0f, 1.0f } },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .X =  0.0f,  0.0f,  1.0f}, { .S = 0.0f, 0.0f } },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .X = -1.0f,  0.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .X = -1.0f,  0.0f,  0.0f}, { .S = 1.0f, 1.0f } },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .X = -1.0f,  0.0f,  0.0f}, { .S = 0.0f, 1.0f } },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .X = -1.0f,  0.0f,  0.0f}, { .S = 0.0f, 1.0f } },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .X = -1.0f,  0.0f,  0.0f}, { .S = 0.0f, 0.0f } },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .X = -1.0f,  0.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .X =  1.0f,  0.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .X =  1.0f,  0.0f,  0.0f}, { .S = 1.0f, 1.0f } },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .X =  1.0f,  0.0f,  0.0f}, { .S = 0.0f, 1.0f } },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .X =  1.0f,  0.0f,  0.0f}, { .S = 0.0f, 1.0f } },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .X =  1.0f,  0.0f,  0.0f}, { .S = 0.0f, 0.0f } },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .X =  1.0f,  0.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .X =  0.0f, -1.0f,  0.0f}, { .S = 0.0f, 1.0f } },
	{ { .X =  0.5f, -0.5f, -0.5f }, { .X =  0.0f, -1.0f,  0.0f}, { .S = 1.0f, 1.0f } },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .X =  0.0f, -1.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X =  0.5f, -0.5f,  0.5f }, { .X =  0.0f, -1.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X = -0.5f, -0.5f,  0.5f }, { .X =  0.0f, -1.0f,  0.0f}, { .S = 0.0f, 0.0f } },
	{ { .X = -0.5f, -0.5f, -0.5f }, { .X =  0.0f, -1.0f,  0.0f}, { .S = 0.0f, 1.0f } },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .X =  0.0f,  1.0f,  0.0f}, { .S = 0.0f, 1.0f } },
	{ { .X =  0.5f,  0.5f, -0.5f }, { .X =  0.0f,  1.0f,  0.0f}, { .S = 1.0f, 1.0f } },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .X =  0.0f,  1.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X =  0.5f,  0.5f,  0.5f }, { .X =  0.0f,  1.0f,  0.0f}, { .S = 1.0f, 0.0f } },
	{ { .X = -0.5f,  0.5f,  0.5f }, { .X =  0.0f,  1.0f,  0.0f}, { .S = 0.0f, 0.0f } },
	{ { .X = -0.5f,  0.5f, -0.5f }, { .X =  0.0f,  1.0f,  0.0f}, { .S = 0.0f, 1.0f } },
};

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

#define HALF_GRID 1000.f
#define FAR       1000.f
#define NEAR        0.1f

static const Vertex unit_quad_horiz_vertices[] = {
	{ { .X = -HALF_GRID, 0.f, -HALF_GRID }, {0}, {0} },
	{ { .X =  HALF_GRID, 0.f, -HALF_GRID }, {0}, {0} },
	{ { .X = -HALF_GRID, 0.f,  HALF_GRID }, {0}, {0} },
	{ { .X =  HALF_GRID, 0.f,  HALF_GRID }, {0}, {0} },
};

static const INDEX_TYPE unit_quad_horiz[] = {
	0, 1, 2,
	1, 3, 2,
};

typedef struct {
	enum {
		TEXTURE_DIFFUSE,
		TEXTURE_SPECULAR,
	} kind;
	sg_view view;
} Texture;

typedef struct {
	Vertex *vertices;
	usize n_vertices;
	INDEX_TYPE *indices;
	usize n_indices;
	Texture *textures;
	usize n_textures;
	sg_bindings bindings;
	HMM_Mat4 model_matrix;
	HMM_Mat4 normal_matrix;
} Mesh;

static void
mesh_default_texture_views(
	Mesh *mesh,
	sg_image diffuse_img,
	sg_image specular_img
) {
	mesh->n_textures = 2;
	mesh->textures = malloc(sizeof(*mesh->textures) * mesh->n_textures);
	mesh->textures[0] = (Texture){
		.kind = TEXTURE_DIFFUSE,
		.view = sg_make_view(&(sg_view_desc){
			.texture.image = diffuse_img,
			.label = "diffuse-texture",
		}),
	};
	mesh->textures[1] = (Texture){
		.kind = TEXTURE_SPECULAR,
		.view = sg_make_view(&(sg_view_desc){
			.texture.image = specular_img,
			.label = "specular-texture",
		}),
	};
}

static void
init_mesh_pipeline(
	sg_pipeline *pipeline
) {
	*pipeline = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = sg_make_shader(scene_shader_desc(sg_query_backend())),
		.layout = {
			// attributes map to fields of `Vertex`
			.attrs = {
				[ATTR_scene_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				[ATTR_scene_aNormal].format = SG_VERTEXFORMAT_FLOAT3,
				[ATTR_scene_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2,
			},
			// computed (default) stride should equal `sizeof(Vertex)`
		},
#if MESH_OUTLINE
		.stencil = {
			.enabled = true,
			.ref = 0x01,
			.read_mask = 0xFF,
			.write_mask = 0xFF,
			.front = {
				.compare = SG_COMPAREFUNC_ALWAYS,
				.pass_op  = SG_STENCILOP_REPLACE,
			},
			.back = {
				.compare = SG_COMPAREFUNC_ALWAYS,
				.pass_op  = SG_STENCILOP_REPLACE,
			},
		},
#endif
		.depth = {
			.write_enabled = true,
			.compare = SG_COMPAREFUNC_LESS,
		},
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
		.index_type = SG_INDEXTYPE_UINT16, // should match `INDEX_TYPE`
		.label = "mesh-pipeline",
	});
}

static void
init_mesh_outline_pipeline(
	sg_pipeline *pipeline
) {
	*pipeline = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = sg_make_shader(outline_shader_desc(sg_query_backend())),
		.layout = {
			.attrs = {
				[ATTR_outline_aPos].format = SG_VERTEXFORMAT_FLOAT3,
			},
			.buffers = {
				[ATTR_outline_aPos].stride = sizeof(Vertex),
			},
		},
		.stencil = {
			.enabled = true,
			.ref = 0x01,
			.read_mask = 0xFF,
			.write_mask = 0x00,
			.front = {
				.compare = SG_COMPAREFUNC_NOT_EQUAL,
				.pass_op = SG_STENCILOP_ZERO,
			},
			.back = {
				.compare = SG_COMPAREFUNC_NOT_EQUAL,
				.pass_op = SG_STENCILOP_ZERO,
			},
		},
		.depth = {
			.write_enabled = true,
			.compare = SG_COMPAREFUNC_ALWAYS,
		},
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
		.index_type = SG_INDEXTYPE_UINT16, // should match `INDEX_TYPE`
		.label = "mesh-outline-pipeline",
	});
}

typedef struct {
	f32 pitch;
	f32 yaw;
	f32 fov;
	HMM_Vec3 pos;
} Camera;

#define CAMERA_UP ((HMM_Vec3){ .X = 0.f, .Y = 1.f, .Z = 0.f })

typedef struct {
	Mesh mesh;
	sg_pipeline mesh_pipeline;
	sg_pipeline mesh_outline_pipeline;
	sg_pipeline light_cube_pipeline;
	sg_bindings light_cube_bindings;
	sg_pipeline horiz_grid_pipeline;
	sg_bindings horiz_grid_bindings;
	sg_pipeline fullscreen_quad_pipeline;
	sg_bindings fullscreen_quad_bindings;
	sg_pipeline skybox_pipeline;
	sg_bindings skybox_bindings;
	sg_view color_attachment;
	sg_view depth_stencil_attachment;
	Camera camera;
	f32 delta_time;
	bool window_focused;
	bool animation_paused;
} AppState;

static const DirLight_t dir_lights[] = {
	{
		.direction = { .X = -0.2f , -1.f  , -0.3f  },
		.ambient   = { .R =  0.4f,  0.4f,  0.4f },
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

static void
mesh_init_bindings(
	Mesh *mesh
) {
	assert(mesh->n_textures < countof(mesh->bindings.views));

	mesh->bindings = (sg_bindings){
		.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.usage.immutable = true,
			.data = (sg_range){
				.ptr = mesh->vertices,
				.size = sizeof(*mesh->vertices) * mesh->n_vertices,
			},
			.label = "mesh-vertex-buffer",
		}),
		.index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.index_buffer = true,
			.usage.immutable = true,
			.data = (sg_range){
				.ptr = mesh->indices,
				.size = sizeof(*mesh->indices) * mesh->n_indices,
			},
			.label = "mesh-index-buffer",
		}),
		.samplers[SMP_texSampler] = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
			.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
			.label = "texture-sampler"
		}),
		.views[VIEW_in_dir_lights] = sg_make_view(&(sg_view_desc){
			.storage_buffer =  {
				.buffer = sg_make_buffer(&(sg_buffer_desc){
					.usage.storage_buffer = true,
					.data = SG_RANGE(dir_lights),
					.label = "directional-lights",
				}),
			},
			.label = "directional-lights-storage",
		}),
		.views[VIEW_in_point_lights] = sg_make_view(&(sg_view_desc){
			.storage_buffer =  {
				.buffer = sg_make_buffer(&(sg_buffer_desc){
					.usage.storage_buffer = true,
					.data = SG_RANGE(point_lights),
					.label = "point-lights",
				}),
			},
			.label = "point-lights-storage",
		}),
	};
	bool diffuse_found = false, specular_found = false;
	for (usize i = 0; i < mesh->n_textures && i < countof(mesh->bindings.views); ++i) {
		Texture *texture = &mesh->textures[i];
		if (SG_RESOURCESTATE_VALID != sg_query_view_state(texture->view)) {
			assert(false);
			continue;
		}
		switch (texture->kind) {
		case TEXTURE_DIFFUSE:
			if (diffuse_found) {
				// TODO(luke): textures should not be limited to only one slot
				assert(false);
				goto next;
			}
			mesh->bindings.views[VIEW_diffuseTexture] = texture->view;
			diffuse_found = true;
			break;
		case TEXTURE_SPECULAR:
			if (specular_found) {
				// TODO(luke): textures should not be limited to only one slot
				assert(false);
				goto next;
			}
			mesh->bindings.views[VIEW_specularTexture] = texture->view;
			specular_found = true;
			break;
		}
next:;
	}
}

static void load_image(const char *filename, bool flip, sg_image *img);

static void
load_obj_as_mesh(
	Mesh *mesh,
	const char *filename
) {
	fastObjMesh *read = fast_obj_read(filename);

	assert(read->position_count < UINT16_MAX); // should match `INDEX_TYPE`
	mesh->n_vertices = read->position_count;
	mesh->n_indices  = read->face_count * 3;
	mesh->vertices = malloc(sizeof(*mesh->vertices) * mesh->n_vertices);
	mesh->indices  = malloc(sizeof(*mesh->indices) * mesh->n_indices);

	for (usize i = 0; i < read->face_count * 3; ++i) {
		fastObjIndex *index = read->indices + i;
		mesh->indices[i] = (INDEX_TYPE)index->p;
		mesh->vertices[index->p] = (Vertex){
			.pos  = *(HMM_Vec3 *)(read->positions + index->p * 3),
			.norm = *(HMM_Vec3 *)(read->normals   + index->n * 3),
			.tex  = *(HMM_Vec2 *)(read->texcoords + index->t * 2),
		};
	}

	fast_obj_destroy(read);
}

#if 0
static void
mesh_default_vertex_data(
	Mesh *mesh
) {
	mesh->n_vertices = countof(cube_vertices);
	mesh->vertices = cube_vertices;
}
#endif

static void
init_mesh(
	Mesh *mesh
) {
	*mesh = (Mesh){0};
	load_obj_as_mesh(mesh, "assets/backpack.obj");

	sg_image diffuse_img, specular_img;
	load_image("assets/backpack_diffuse.jpg", false, &diffuse_img);
	load_image("assets/backpack_specular.jpg", false, &specular_img);

	mesh_default_texture_views(mesh, diffuse_img, specular_img);
	mesh_init_bindings(mesh);

	mesh->model_matrix = HMM_Mul(HMM_Translate(HMM_V3(0.f, 0.f, 0.f)),
	                             HMM_Rotate_RH(HMM_AngleDeg(0.f), HMM_V3(1.f, 0.3f, 0.5f)));
	mesh->normal_matrix = HMM_Transpose(HMM_InvGeneral(mesh->model_matrix));
}

static void
deinit_mesh(
	Mesh *mesh
) {
	if (mesh->textures) {
		free(mesh->textures);
	}
	mesh->textures = NULL;
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
	return HMM_Perspective_RH_NO(HMM_AngleDeg(camera->fov), aspect_ratio, NEAR, FAR);
}

static void
draw_mesh(
	sg_pipeline *pipeline,
	Mesh *mesh,
	Camera *camera
) {
	sg_apply_pipeline(*pipeline);
	sg_apply_bindings(&mesh->bindings);

	scene_material_t scene_material = {
		.mat_shininess = 32.f,
	};
	sg_apply_uniforms(UB_scene_material, &SG_RANGE(scene_material));

	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(camera->pitch, camera->yaw);
	scene_vs_params_t scene_vs_params = {
		.view       = HMM_LookAt_RH(camera->pos, HMM_Add(camera->pos, camera_front), CAMERA_UP),
		.projection = projection_matrix(camera),
		.model      = mesh->model_matrix,
		.normal     = mesh->normal_matrix,
	};
	sg_apply_uniforms(UB_scene_vs_params, &SG_RANGE(scene_vs_params));

	scene_fs_params_t scene_fs_params = {
		.viewPos = camera->pos,
		.n_dir_lights = countof(dir_lights),
		.n_point_lights = countof(point_lights),
	};
	sg_apply_uniforms(UB_scene_fs_params, &SG_RANGE(scene_fs_params));

	sg_draw(0, mesh->n_indices, 1);
}

static void
draw_skybox(
	sg_pipeline *pipeline,
	sg_bindings *bindings,
	Camera *camera
) {
	sg_apply_pipeline(*pipeline);
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
draw_mesh_outline(
	sg_pipeline *pipeline,
	Mesh *mesh,
	Camera *camera
) {
#if MESH_OUTLINE
#else
	return;
#endif
	sg_apply_pipeline(*pipeline);
	sg_apply_bindings(&mesh->bindings);

	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(camera->pitch, camera->yaw);
	outline_vs_params_t outline_vs_params = {
		.view       = HMM_LookAt_RH(camera->pos, HMM_Add(camera->pos, camera_front), CAMERA_UP),
		.projection = projection_matrix(camera),
		.model      = HMM_Mul(mesh->model_matrix, HMM_Scale(HMM_V3(1.1f, 1.1f, 1.1f))),
	};
	sg_apply_uniforms(UB_outline_vs_params, &SG_RANGE(outline_vs_params));

	outline_fs_params_t outline_fs_params = {
		.outline_color = HMM_V3(0.04f, 0.28f, 0.26f),
	};
	sg_apply_uniforms(UB_outline_fs_params, &SG_RANGE(outline_fs_params));

	sg_draw(0, mesh->n_indices, 1);
}

static void
draw_light_cubes(
	sg_pipeline *pipeline,
	sg_bindings *bindings,
	Camera *camera
) {
	sg_apply_pipeline(*pipeline);
	sg_apply_bindings(bindings);

	HMM_Vec3 camera_front = cam_direction_from_pitch_yaw(camera->pitch, camera->yaw);
	light_vs_params_t light_vs_params = {
		.view       = HMM_LookAt_RH(camera->pos, HMM_Add(camera->pos, camera_front), CAMERA_UP),
		.projection = projection_matrix(camera),
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
}

static void
draw_horiz_grid(
	sg_pipeline *pipeline,
	sg_bindings *bindings,
	Camera *camera
) {
#if GRID
#else
	return;
#endif
	sg_apply_pipeline(*pipeline);
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
		.model      = HMM_Translate(grid_pos),
	};

	sg_apply_uniforms(UB_grid_vs_params, &SG_RANGE(grid_vs_params));
	grid_fs_params_t grid_fs_params = {
		.grid_color = HMM_V3(0.5f, 0.5f, 0.5f),
	};
	sg_apply_uniforms(UB_grid_fs_params, &SG_RANGE(grid_fs_params));
	sg_draw(0, countof(unit_quad_horiz), 1);
}

static
void load_image(
	const char *filename,
	bool flip,
	sg_image *img
) {
	stbi_set_flip_vertically_on_load(flip);
	int w, h, n_channels;
	byte *img_data = stbi_load(filename, &w, &h, &n_channels, 4);
	if (img_data) {
		*img = sg_make_image(&(sg_image_desc){
			.width = w,
			.height = h,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
			.data.mip_levels[0] = {
				.ptr = img_data,
				.size = (size_t)(w * h * 4),
			},
		});
		stbi_image_free(img_data);
	} else {
		assert(false);
	}
}

static
void init_skybox(
	sg_image *img
) {
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
	*img = sg_make_image(&(sg_image_desc){
		.type = SG_IMAGETYPE_CUBE,
		.width = w_expected,
		.height = h_expected,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.num_slices = 6,
		.data.mip_levels[0] = { .ptr = bytes, .size = cube_texture_sz, },
	});
	free(bytes);
}

#define optional(x) x *

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
		.sample_count = 1,
	});
	sg_image depth_stencil_attachment_img = sg_make_image(&(sg_image_desc){
		.usage.depth_stencil_attachment = true,
		.usage.immutable = true,
		.width = sapp_width(),
		.height = sapp_height(),
		.sample_count = 1,
	});

	sg_image skybox_img;
	init_skybox(&skybox_img);

	*state = (AppState){
		.light_cube_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(light_shader_desc(sg_query_backend())),
			.layout = (sg_vertex_layout_state){
				.attrs = {
					[ATTR_light_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				},
				.buffers = {
					[ATTR_light_aPos].stride = sizeof(Vertex),
				},
			},
			.depth = (sg_depth_state){
				.write_enabled = true,
				.compare = SG_COMPAREFUNC_LESS,
			},
			.label = "light-cube-pipeline",
		}),
		.light_cube_bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.usage.immutable = true,
			.data = SG_RANGE(cube_vertices),
			.label = "cube-vertex-buffer",
		}),
		.horiz_grid_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(grid_shader_desc(sg_query_backend())),
			.layout = (sg_vertex_layout_state){
				.attrs = {
					[ATTR_grid_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				},
				.buffers = {
					[ATTR_grid_aPos].stride = sizeof(Vertex),
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
		.horiz_grid_bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.usage.immutable = true,
			.data = SG_RANGE(unit_quad_horiz_vertices),
			.label = "grid-vertex-buffer",
		}),
		.horiz_grid_bindings.index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.index_buffer = true,
			.usage.immutable = true,
			.data = SG_RANGE(unit_quad_horiz),
			.label = "grid-index-buffer",
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
				.texture.image = color_attachment_img,
				.label = "offscreen-texture-view",
			}),
		},
		.skybox_pipeline = sg_make_pipeline(&(sg_pipeline_desc){
			.shader = sg_make_shader(skybox_shader_desc(sg_query_backend())),
			.layout = (sg_vertex_layout_state){
				// attributes map to fields of `SkyboxVertex`
				.attrs = {
					[ATTR_skybox_aPos].format = SG_VERTEXFORMAT_FLOAT3,
				},
			},
			.depth = (sg_depth_state){
				.write_enabled = false,
				.compare = SG_COMPAREFUNC_LESS_EQUAL,
			},
			.label = "skybox-pipeline",
		}),
		.skybox_bindings = (sg_bindings){
			.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
				.usage.vertex_buffer = true,
				.usage.immutable = true,
				.data = SG_RANGE(skybox_vertices),
				.label = "skybox-vertex-buffer",
			}),
			.samplers[SMP_skyboxSampler] = sg_make_sampler(&(sg_sampler_desc){
				.min_filter = SG_FILTER_LINEAR,
				.mag_filter = SG_FILTER_LINEAR,
				.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
				.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
				.wrap_w = SG_WRAP_CLAMP_TO_EDGE,
				.label = "skybox-samplers"
			}),
			.views[0] = sg_make_view(&(sg_view_desc){
				.texture = {
					.image = skybox_img,
				},
				.label = "skybox-texture",
			}),
		},
		.color_attachment = sg_make_view(&(sg_view_desc){
			.color_attachment.image = color_attachment_img,
			.label = "color-attachment-view",
		}),
		.depth_stencil_attachment = sg_make_view(&(sg_view_desc){
			.depth_stencil_attachment.image = depth_stencil_attachment_img,
			.label = "depth-stencil-attachment-view",
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

	init_mesh(&state->mesh);
	init_mesh_pipeline(&state->mesh_pipeline);
	init_mesh_outline_pipeline(&state->mesh_outline_pipeline);
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

	float theta = stm_sec(current_frame - time_paused) * 100.;
	state->mesh.model_matrix = HMM_Mul(HMM_Translate(HMM_V3(0.f, 0.f, 0.f)),
	                                   HMM_Rotate_RH(HMM_AngleDeg(-theta), HMM_V3(0.f, 1.0f, 0.0f)));
	state->mesh.normal_matrix = HMM_Transpose(HMM_InvGeneral(state->mesh.model_matrix));

	sg_begin_pass(&(sg_pass){
		.action = (sg_pass_action) {
			.colors[0] = {
				.load_action = SG_LOADACTION_CLEAR,
				.store_action = SG_STOREACTION_STORE,
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
			.colors[0] = state->color_attachment, // how to tell it to render to this one?
			.depth_stencil = state->depth_stencil_attachment,
		},
	});

	draw_mesh(&state->mesh_pipeline, &state->mesh, &state->camera);
	draw_mesh_outline(&state->mesh_outline_pipeline, &state->mesh, &state->camera);
	draw_light_cubes(&state->light_cube_pipeline, &state->light_cube_bindings, &state->camera);
	draw_skybox(&state->skybox_pipeline, &state->skybox_bindings, &state->camera);
	draw_horiz_grid(&state->horiz_grid_pipeline, &state->horiz_grid_bindings, &state->camera);

	sg_end_pass();

	sg_begin_pass(&(sg_pass){
		.action = (sg_pass_action) {
			.colors[0] = {
				.load_action = SG_LOADACTION_DONTCARE,
				.store_action = SG_STOREACTION_STORE,
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
	AppState *state = state_;
	sg_shutdown();
	deinit_mesh(&state->mesh);
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
