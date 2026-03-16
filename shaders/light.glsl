@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4

@vs light_vs
layout(binding=0) uniform light_vs_params {
	mat4 model;
	mat4 view;
	mat4 projection;
};
in vec3 aPos;

void main(
	void
) {
	gl_Position = projection * view * model * vec4(aPos, 1.);
}
@end

@fs light_fs
out vec4 FragColor;

void main(
	void
) {
	FragColor = vec4(1.);
}
@end

@program light light_vs light_fs

