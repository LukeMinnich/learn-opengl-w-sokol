#include "mesh.h"

#include "render.h"
#include "mesh.glsl.h"

#include "handmade_math.h"

#include "sokol_gfx.h"
#include "texture.h"

#include <assert.h>

void
init_mesh_shadow_pipeline(
	sg_pipeline *pipeline
) {
	assert(!pipeline->id);
	*pipeline = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = sg_make_shader(shadow_shader_desc(sg_query_backend())),
		.layout = {
			.buffers[0] = { .stride = sizeof(MeshVertex) },
			.buffers[1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
			.attrs[ATTR_shadow_position]  = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			.attrs[ATTR_shadow_iposition] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 1 },
		},
		.depth = {
			.write_enabled = true,
			.compare = SG_COMPAREFUNC_LESS,
			.pixel_format = SG_PIXELFORMAT_DEPTH,
		},
		/* Only render back-faces to prevent shadow acne on front-faces. */
		.cull_mode = SG_CULLMODE_FRONT,
		.face_winding = SG_FACEWINDING_CCW,
		.sample_count = SHADOW_SAMPLE_COUNT,
		.index_type = MESH_INDEX_TYPE,
		/* Deactivate the default color target for 'depth-only-rendering'. */
		.colors[0].pixel_format = SG_PIXELFORMAT_NONE,
		.label = "mesh-render-pipeline",
	});
}

void
init_mesh_display_pipeline(
	sg_pipeline *pipeline
) {
	assert(!pipeline->id);
	*pipeline = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = sg_make_shader(display_shader_desc(sg_query_backend())),
		.layout = {
			.buffers[0] = { .stride = sizeof(MeshVertex) },
			.buffers[1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
			.attrs[ATTR_display_position]  = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			.attrs[ATTR_display_normal]    = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			.attrs[ATTR_display_tangent]   = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			.attrs[ATTR_display_tex_coord] = { .format = SG_VERTEXFORMAT_FLOAT2, .buffer_index = 0 },
			.attrs[ATTR_display_iposition] = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 1 },
		},
		.depth = {
			.write_enabled = true,
			.compare = SG_COMPAREFUNC_LESS,
		},
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
		.index_type = MESH_INDEX_TYPE,
		.sample_count = OFFSCREEN_SAMPLE_COUNT,
		.label = "mesh-display-pipeline",
	});
}

void
mesh_fixup_bivectors(
	MeshVertexView *vertices,
	MeshTriangleView *triangles,
	bool fixup_normals
) {
	for (usize i = 0; i < triangles->len; ++i) {
		MeshTriangle *triangle = &triangles->ptr[i];
		MeshVertex *v0 = &vertices->ptr[triangle->indices[0]];
		MeshVertex *v1 = &vertices->ptr[triangle->indices[1]];
		MeshVertex *v2 = &vertices->ptr[triangle->indices[2]];
		HMM_Vec3 e1 = HMM_Sub(v1->position , v0->position);
		HMM_Vec3 e2 = HMM_Sub(v2->position , v0->position);
		HMM_Vec2 d1 = HMM_Sub(v1->tex_coord, v0->tex_coord);
		HMM_Vec2 d2 = HMM_Sub(v2->tex_coord, v0->tex_coord);
		f32 f = 1.f / (d1.X * d2.Y - d2.X * d1.Y);
		HMM_Vec3 tangent = HMM_Norm(HMM_V3(
			f * (d2.Y * e1.X - d1.Y * e2.X),
			f * (d2.Y * e1.Y - d1.Y * e2.Y),
			f * (d2.Y * e1.Z - d1.Y * e2.Z)
		));
		v0->tangent = tangent;
		v1->tangent = tangent;
		v2->tangent = tangent;
		if (fixup_normals) {
			HMM_Vec3 normal = HMM_Norm(HMM_Cross(e1, e2));
			v0->normal = normal;
			v1->normal = normal;
			v2->normal = normal;
		}
		/* If not fixing up normals, we'll need to re-orthogonalize the TBN vectors in the vertex
		   shader */
	}
}

void
	mesh_gen_primitive_tetrahedron(
	Mesh *mesh
) {
	f32 a = 1.f / 3.f;
	f32 b = sqrtf(8.f / 9.f);
	f32 c = sqrtf(2.f / 9.f);
	f32 d = sqrtf(2.f / 3.f);

	HMM_Vec3 top   = { .X =  0,  1,  0 };
	HMM_Vec3 bot0  = { .X =  0, -a,  b };
	HMM_Vec3 bot1  = { .X = -d, -a, -c };
	HMM_Vec3 bot2  = { .X =  d, -a, -c };

	HMM_Vec2 t0 = { .S = 0, 0 };
	HMM_Vec2 t1 = { .S = 1, 0 };
	HMM_Vec2 t2 = { .S = 0.5f, 0.5f * sqrtf(3.f) };

	MeshVertex vertices[] = {
		/* NOTE(luke): omit normals and tangents. */
		{ .position = top , .tex_coord = t0 },
		{ .position = bot2, .tex_coord = t1 },
		{ .position = bot1, .tex_coord = t2 },

		{ .position = top , .tex_coord = t0 },
		{ .position = bot1, .tex_coord = t1 },
		{ .position = bot0, .tex_coord = t2 },

		{ .position = top , .tex_coord = t0 },
		{ .position = bot0, .tex_coord = t1 },
		{ .position = bot2, .tex_coord = t2 },

		{ .position = bot0, .tex_coord = t0 },
		{ .position = bot1, .tex_coord = t1 },
		{ .position = bot2, .tex_coord = t2 },
	};

	MeshTriangle triangles[] = {
		 {0,  1,  2},
		 {3,  4,  5},
		 {6,  7,  8},
		 {9, 10, 11},
	};

	mesh_fixup_bivectors(&av(MeshVertex, vertices), &av(MeshTriangle, triangles), true);
	mesh_init_raw(&av(MeshVertex, vertices), &av(MeshTriangle, triangles), mesh);
}


void
mesh_gen_primitive_cube(
	Mesh *mesh
) {
	MeshVertex vertices[24];
	MeshTriangle triangles[12];

	// right, front, top
	// left , back , bot

	float dim = 0.5f;

	for (usize i = 0; i < 6; ++i) {
		bool x_facing = 0 == (i % 3);
		bool z_facing = 1 == (i % 3);
		bool y_facing = 2 == (i % 3);

		HMM_Vec2 t0 = HMM_V2(0.f, 0.f);
		HMM_Vec2 t1 = HMM_V2(1.f, 0.f);
		HMM_Vec2 t2 = HMM_V2(0.f, 1.f);
		HMM_Vec2 t3 = HMM_V2(1.f, 1.f);

		float x, y, z;
		HMM_Vec3 p0, p1, p2, p3;

		float sign = i > 2 ? -1.f : 1.f;

		x = dim * sign;
		y = dim;
		z = dim * sign;

		if (x_facing) {
			p0 = HMM_V3(x, -y,  z);
			p1 = HMM_V3(x, -y, -z);
			p2 = HMM_V3(x,  y,  z);
			p3 = HMM_V3(x,  y, -z);
		} else if (z_facing) {
			p0 = HMM_V3( x, -y, -z);
			p1 = HMM_V3(-x, -y, -z);
			p2 = HMM_V3( x,  y, -z);
			p3 = HMM_V3(-x,  y, -z);
		} else {
			assert(y_facing);
			(void)y_facing;
			p0 = HMM_V3( x, y * sign, -z * sign);
			p1 = HMM_V3(-x, y * sign, -z * sign);
			p2 = HMM_V3( x, y * sign,  z * sign);
			p3 = HMM_V3(-x, y * sign,  z * sign);
		}

		MeshIndex off = (MeshIndex)i * 4;
		vertices[off + 0] = (MeshVertex){ .position = p0, .tex_coord = t0 };
		vertices[off + 1] = (MeshVertex){ .position = p1, .tex_coord = t1 };
		vertices[off + 2] = (MeshVertex){ .position = p2, .tex_coord = t2 };
		vertices[off + 3] = (MeshVertex){ .position = p3, .tex_coord = t3 };

		triangles[i * 2 + 0] = (MeshTriangle){ { off + 0, off + 1, off + 2 } };
		triangles[i * 2 + 1] = (MeshTriangle){ { off + 2, off + 1, off + 3 } };
	}

	mesh_fixup_bivectors(&av(MeshVertex, vertices), &av(MeshTriangle, triangles), true);
	mesh_init_raw(&av(MeshVertex, vertices), &av(MeshTriangle, triangles), mesh);
}

void
mesh_gen_primitive_plane(
	Mesh *mesh
) {
	float dim = 0.5f;
	MeshVertex vertices[] = {
		{ .position = HMM_V3(-dim, -dim, 0.f), .tex_coord = HMM_V2(0.f, 0.f) },
		{ .position = HMM_V3( dim, -dim, 0.f), .tex_coord = HMM_V2(1.f, 0.f) },
		{ .position = HMM_V3(-dim,  dim, 0.f), .tex_coord = HMM_V2(0.f, 1.f) },
		{ .position = HMM_V3( dim,  dim, 0.f), .tex_coord = HMM_V2(1.f, 1.f) },
	};
	MeshTriangle triangles[] = { {0, 1, 2}, {2, 1, 3} };

	mesh_fixup_bivectors(&av(MeshVertex, vertices), &av(MeshTriangle, triangles), true);
	mesh_init_raw(&av(MeshVertex, vertices), &av(MeshTriangle, triangles), mesh);
}

typedef sg_image (*default_texture_gen)(void);
#include "texture_kind.h"
#define X_TEXTURE_KINDS(kind, _, gen, ...) [kind] = gen,
static const default_texture_gen default_texture_images[] = { TEXTURE_KINDS };

bool
texture_image_is_default(
	sg_image img,
	TextureKind kind
) {
	return img.id == default_texture_images[kind]().id;
}

void
mesh_set_texture_defaults(
	Mesh *mesh
) {
	#include "texture_kind.h"
	#define X_TEXTURE_KINDS(kind, _, gen, ...)    \
	  mesh->textures[kind] = gen##_texture(kind); \
	  mesh->texture_images[kind] = default_texture_images[kind]();
	TEXTURE_KINDS
}

void
mesh_set_texture(
	Mesh *mesh,
	sg_image img,
	TextureKind kind
) {
	if (   !mesh->textures[kind].id
	    && !texture_image_is_default(mesh->texture_images[kind], kind)) {
		assert(false && "texture already set to non-default");
	}
	mesh->textures[kind] = sg_make_view(&(sg_view_desc){
		.texture.image = img,
		.label = str_from_texture_kind(kind).ptr,
	});
}

void
mesh_init_raw(
	MeshVertexView *vertices,
	MeshTriangleView *triangles,
	Mesh *mesh
) {
	assert(!mesh->vertex_buffer.id);
	assert(!mesh->index_buffer.id);
	*mesh = (Mesh){
		.vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.usage.immutable = true,
			.data = { .ptr = vertices->ptr, .size = vertices->len * sizeof(*vertices->ptr) },
			.label = "mesh-vertex-buffer",
		}),
		.index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.index_buffer = true,
			.usage.immutable = true,
			.data = { .ptr = triangles->ptr, .size = triangles->len * sizeof(*triangles->ptr) },
			.label = "mesh-index-buffer",
		}),
		.num_elements = triangles->len * countof(((MeshTriangle *)NULL)->indices),
	};
	mesh_set_texture_defaults(mesh);
}

MeshInstances
mesh_instances(
	usize num_instances
) {
	assert(num_instances > 1 && "use `mesh_one_instance_buffer()` instead");
	return (MeshInstances){
		.buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.usage.stream_update = true,
			.size = sizeof(MeshInstanceData) * num_instances,
			.label = "many-instances-buffer",
		}),
		.num_instances = num_instances,
	};
}

static sg_buffer one_instance_buffer = {0};
static MeshInstanceData one_instance_no_translation[1] = {{0}};

sg_buffer
mesh_one_instance_buffer(
	void
) {
	if (!one_instance_buffer.id) {
		one_instance_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.vertex_buffer = true,
			.usage.immutable = true,
			.data = SG_RANGE(one_instance_no_translation),
			.label = "one-instance-buffer"
		});
	}
	return one_instance_buffer;
}

void
mesh_apply_textures_to_bindings(
	Mesh *mesh,
	sg_bindings *bindings
) {
	for (TextureKind kind = 0; kind < TextureKindCount; ++kind) {
		sg_view texture = mesh->textures[kind];
		assert(sg_query_view_state(texture));
		switch (kind) {
	  case TextureKindDiffuse:
	  	bindings->views[VIEW_diffuseTexture] = texture;
		  break;
	  case TextureKindSpecular:
	  	bindings->views[VIEW_specularTexture] = texture;
		  break;
	  case TextureKindNone:
	  case TextureKindEmissive:
	  case TextureKindNormal:
	  case TextureKindParallax:
	  	assert(   texture_image_is_default(mesh->texture_images[kind], kind)
	  	       && "implement me!");
		  break;
	  }
	}
}
