@header #include "sokol_gfx.h"

@vs scene_vs
in vec3 aPos;
in vec3 aColor;

out vec3 ourColor;

void main(
	void
) {
	gl_Position = vec4(aPos, 1.);
	ourColor = aColor;
}
@end

@fs scene_fs
out vec4 FragColor;
in vec3 ourColor;

void main(
	void
) {
	FragColor = vec4(ourColor, 1.);
}
@end

@program scene scene_vs scene_fs

