#include "mesh.h"

#include "render.h"
#include "mesh.glsl.h"

#include "handmade_math.h"

#include "sokol_gfx.h"

#include <assert.h>

void
mesh_shadow_pipeline(
	sg_pipeline *pipeline
) {
	assert(!pipeline->id);
	*pipeline = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = sg_make_shader(shadow_shader_desc(sg_query_backend())),
		.layout = {
			.buffers[0].stride = sizeof(MeshVertex),
			.attrs[ATTR_shadow_position]  = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			// TODO(luke): bind per-instance data here
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
mesh_display_pipeline(
	sg_pipeline *pipeline
) {
	assert(!pipeline->id);
	*pipeline = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = sg_make_shader(display_shader_desc(sg_query_backend())),
		.layout = {
			.attrs[ATTR_display_position]  = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			.attrs[ATTR_display_normal]    = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			.attrs[ATTR_display_tangent]   = { .format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
			.attrs[ATTR_display_tex_coord] = { .format = SG_VERTEXFORMAT_FLOAT2, .buffer_index = 0 },
			// TODO(luke): bind per-instance data here
		},
		.depth = {
			.write_enabled = true,
			.compare = SG_COMPAREFUNC_LESS,
		},
		.cull_mode = SG_CULLMODE_BACK,
		.face_winding = SG_FACEWINDING_CCW,
		.index_type = MESH_INDEX_TYPE,
		.label = "mesh-render-pipeline",
	});
}

typedef struct {
	MeshVertex *d;
	u32 n;
} MeshVertexView;

typedef struct {
	MeshTriangle *d;
	u32 n;
} MeshTriangleView;

#define ArrView(T, a) ((T##View){ .d = (a), .n = countof(a) })

static void
mesh_fixup_vertex_bivectors(
	MeshVertexView *vertices,
	MeshTriangleView *triangles
) {
	for (usize i = 0; i < triangles->n; ++i) {
		MeshTriangle *triangle = &triangles->d[i];
		MeshVertex *v0 = &vertices->d[triangle->indices[0]];
		MeshVertex *v1 = &vertices->d[triangle->indices[1]];
		MeshVertex *v2 = &vertices->d[triangle->indices[2]];
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
		HMM_Vec3 normal = HMM_Norm(HMM_Cross(e1, e2));
		v0->normal = normal;
		v1->normal = normal;
		v2->normal = normal;
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

	mesh_fixup_vertex_bivectors(&ArrView(MeshVertex, vertices),
	                            &ArrView(MeshTriangle, triangles));

	*mesh = (Mesh){
		.vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
			.data = SG_RANGE(vertices),
			.label = "cube-vertex-buffer",
		}),
		.index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.index_buffer = true,
			.data = SG_RANGE(triangles),
			.label = "cube-index-buffer",
		}),
		.num_elements = countof(triangles) * countof(((MeshTriangle *)NULL)->indices),
	};
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

	mesh_fixup_vertex_bivectors(&ArrView(MeshVertex, vertices),
	                            &ArrView(MeshTriangle, triangles));

	*mesh = (Mesh){
		.vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
			.data = SG_RANGE(vertices),
			.label = "cube-vertex-buffer",
		}),
		.index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.index_buffer = true,
			.data = SG_RANGE(triangles),
			.label = "cube-index-buffer",
		}),
		.num_elements = countof(triangles) * countof(((MeshTriangle *)NULL)->indices),
	};
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

	mesh_fixup_vertex_bivectors(&ArrView(MeshVertex, vertices),
	                            &ArrView(MeshTriangle, triangles));

	*mesh = (Mesh){
		.vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
		.data = SG_RANGE(vertices),
		.label = "plane-vertex-buffer",
		}),
		.index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.usage.index_buffer = true,
			.data = SG_RANGE(triangles),
			.label = "plane-index-buffer",
		}),
		.num_elements = countof(triangles) * countof(((MeshTriangle *)NULL)->indices),
	};
}

void
mesh_draw_shadow(
	Mesh *mesh
){
	sg_apply_bindings(&(sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.index_buffer = mesh->index_buffer,
	});
	sg_draw(0, (int)mesh->num_elements, 1);
}

void
mesh_draw_display(
	Mesh *mesh,
	sg_view shadow_map_tex_view,
	sg_sampler shadow_sampler
){
	sg_apply_bindings(&(sg_bindings){
		.vertex_buffers[0] = mesh->vertex_buffer,
		.index_buffer = mesh->index_buffer,
		.views[VIEW_shadow_map] = shadow_map_tex_view,
		.samplers[SMP_shadow_sampler] = shadow_sampler,
	});
	sg_draw(0, (int)mesh->num_elements, 1);
}
