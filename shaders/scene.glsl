@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3

@vs scene_vs
layout(binding=0) uniform scene_vs_params {
	mat4 model;
	mat4 view;
	mat4 projection;
	mat4 normal;
};
in vec3 aPos;
in vec2 aTexCoord;
in vec3 aNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

void main(
	void
) {
	gl_Position = projection * view * model * vec4(aPos, 1.);
	FragPos = (model * vec4(aPos, 1.f)).xyz;
	Normal = mat3(normal) * aNormal;
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
	vec3 lightPos;
	vec3 viewPos;
};

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

void main(
	void
) {
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.);
	vec3 diffuse = diff * lightColor;

	float specularStrength = 0.5;
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.), 32);
	vec3 specular = specularStrength * spec * lightColor;

	vec3 result = (ambient + diffuse + specular) * objectColor;
	FragColor = vec4(result, 1.);
}
@end

@program scene scene_vs scene_fs

