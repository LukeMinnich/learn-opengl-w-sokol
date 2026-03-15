@header #include "sokol_gfx.h"

@vs scene_vs
in vec3 aPos;

void main(
	void
) {
	gl_Position = vec4(aPos, 1.);
}
@end

@fs scene_fs
layout(binding=0) uniform scene_fs_params {
	vec4 ourColor;
};

// in vec4 gl_FragCoord;
out vec4 FragColor;

void main(
	void
) {
	FragColor = ourColor;
}
@end

@program scene scene_vs scene_fs

