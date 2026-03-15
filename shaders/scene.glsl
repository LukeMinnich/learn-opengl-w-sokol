@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4

@vs scene_vs
layout(binding=0) uniform scene_vs_params {
	mat4 transform;
};
in vec3 aPos;
in vec3 aColor;
in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main(
	void
) {
	gl_Position = transform * vec4(aPos, 1.);
	ourColor = aColor;
	TexCoord = aTexCoord;
}
@end

@fs scene_fs
layout(binding=0) uniform texture2D boxTexture;
layout(binding=0) uniform sampler boxSampler;
layout(binding=1) uniform texture2D faceTexture;
layout(binding=1) uniform sampler faceSampler;

in vec3 ourColor;
in vec2 TexCoord;

out vec4 FragColor;

void main(
	void
) {
	FragColor = mix(texture(sampler2D(boxTexture, boxSampler), TexCoord),
                  texture(sampler2D(faceTexture, faceSampler), TexCoord),
                  0.2);
}
@end

@program scene scene_vs scene_fs

