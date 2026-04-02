/* A mesh is a geometric entity whose triangles are rendered with a single draw call. */
#ifndef MESH_H
#define MESH_H

#include "primitives.h"
#include "texture.h"

#include "handmade_math.h"
#include "sokol_gfx.h"

#define MESH_INDEX_TYPE SG_INDEXTYPE_UINT16
typedef u16 MeshIndex; // keep in-sync w/ surrounding
#define MESH_INDEX_MAX UINT16_MAX

typedef struct {
	HMM_Vec3 position;
	HMM_Vec3 normal;
	HMM_Vec3 tangent;
	HMM_Vec2 tex_coord;
} MeshVertex;

typedef struct {
	MeshIndex indices[3];
} MeshTriangle;

typedef struct {
	sg_buffer vertex_buffer;
	sg_buffer index_buffer;
	sg_view textures[TextureKindCount];
	sg_image texture_images[TextureKindCount];
	u32 num_elements;
} Mesh;

void init_mesh_shadow_pipeline(sg_pipeline *pipeline);
void init_mesh_display_pipeline(sg_pipeline *pipeline);

void mesh_gen_primitive_tetrahedron(Mesh *mesh);
void mesh_gen_primitive_cube(Mesh *mesh);
void mesh_gen_primitive_plane(Mesh *mesh);

bool texture_image_is_default(sg_image img, TextureKind kind);
void mesh_set_texture_defaults(Mesh *mesh);
void mesh_set_texture(Mesh *mesh, sg_image img, TextureKind kind);

typedef struct {
	MeshVertex *ptr;
	usize len;
} MeshVertexView;

typedef struct {
	MeshTriangle *ptr;
	usize len;
} MeshTriangleView;

void mesh_init_raw(MeshVertexView *vertices, MeshTriangleView *triangles, Mesh *mesh);
void mesh_fixup_bivectors(MeshVertexView *vertices, MeshTriangleView *triangles, bool fixup_normals);

/* TODO(luke): support initializing a larger buffer than needed (i.e. w/ excess capacity). */
typedef struct {
	sg_buffer buffer;
	usize num_instances;
} MeshInstances;

typedef HMM_Vec3 MeshInstanceData;

MeshInstances mesh_instances(usize num_instances);
sg_buffer mesh_one_instance_buffer(void);

void mesh_apply_textures_to_bindings(Mesh *mesh, sg_bindings *bindings);

#endif // MESH_H
