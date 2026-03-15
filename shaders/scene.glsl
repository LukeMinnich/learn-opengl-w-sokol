@header #include "sokol_gfx.h"

@vs scene_vs
in vec3 aPos;
in vec3 aColor;
in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main(
	void
) {
	gl_Position = vec4(aPos, 1.);
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

