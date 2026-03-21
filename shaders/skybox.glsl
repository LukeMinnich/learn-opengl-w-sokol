@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3

@vs skybox_vs
layout(binding=0) uniform skybox_vs_params {
	mat4 view;
	mat4 projection;
};
in vec3 aPos;
out vec3 TexCoords;

void main(
	void
) {
	TexCoords = vec3(aPos.xy, -aPos.z); // convert cubemap LH coords to RH
	vec4 pos = projection * view * vec4(aPos, 1.);
	/* Trick perspective division into thinking every fragment is at Z == 1.0. Then we can render
	   the skybox after all other opaque meshes (to mitigate some overdraw). */
	gl_Position = pos.xyww;
}
@end

@fs skybox_fs
layout(binding=0) uniform sampler skyboxSampler;
layout(binding=0) uniform textureCube skyboxTexture;

in vec3 TexCoords;
out vec4 FragColor;

void main(
	void
) {
	FragColor = texture(samplerCube(skyboxTexture, skyboxSampler), TexCoords);
}
@end

@program skybox skybox_vs skybox_fs

