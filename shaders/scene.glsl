@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3

@vs scene_vs
layout(binding=0) uniform scene_vs_params {
	mat4 model;
	mat4 view;
	mat4 projection;
};
in vec3 aPos;
in vec2 aTexCoord;

out vec2 TexCoord;

void main(
	void
) {
	gl_Position = projection * view * model * vec4(aPos, 1.);
	TexCoord = aTexCoord;
}
@end

@fs scene_fs
layout(binding=0) uniform texture2D boxTexture;
layout(binding=0) uniform sampler boxSampler;
layout(binding=1) uniform texture2D faceTexture;
layout(binding=1) uniform sampler faceSampler;
layout(binding=1) uniform scene_fs_params {
	vec3 objectColor;
	vec3 lightColor;
};

in vec2 TexCoord;

out vec4 FragColor;

void main(
	void
) {
	FragColor = vec4(lightColor * objectColor, 1.);
}
@end

@program scene scene_vs scene_fs

