/* A mesh is a geometric entity whose triangles are rendered with a single draw call. */
#ifndef MESH_H
#define MESH_H

#include "primitives.h"
#include "texture.h"

#include "handmade_math.h"
#include "sokol_gfx.h"

#define MESH_INDEX_TYPE SG_INDEXTYPE_UINT16
typedef u16 MeshIndex; // keep in-sync w/ ^

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
	u32 num_elements;
} Mesh;

void mesh_shadow_pipeline(sg_pipeline *pipeline);
void mesh_display_pipeline(sg_pipeline *pipeline);

void mesh_gen_primitive_tetrahedron(Mesh *mesh);
void mesh_gen_primitive_cube(Mesh *mesh);
void mesh_gen_primitive_plane(Mesh *mesh);

void mesh_draw_shadow(Mesh *mesh);
void mesh_draw_display(Mesh *mesh, sg_view shadow_map_tex_view, sg_sampler shadow_sampler);

#endif // MESH_H
