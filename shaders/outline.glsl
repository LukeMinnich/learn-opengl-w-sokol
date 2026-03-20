@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3

@vs outline_vs
layout(binding=0) uniform outline_vs_params {
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

@fs outline_fs
layout(binding=1) uniform outline_fs_params {
	vec3 outline_color;
};
out vec4 FragColor;

void main(
	void
) {
	FragColor = vec4(outline_color, 1.f);
}
@end

@program outline outline_vs outline_fs

