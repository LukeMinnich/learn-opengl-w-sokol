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
layout(binding=0) uniform sampler boxSampler; // there only needs to be one of these for many
                                              // material texture lookups
layout(binding=1) uniform scene_fs_params {
	vec3 viewPos;
	int n_dir_lights;
	int n_point_lights;
};
layout(binding=2) uniform scene_material {
	float mat_shininess;
};

layout(binding=0) uniform texture2D diffuseTexture;
layout(binding=1) uniform texture2D specularTexture;
struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
layout(binding=2) readonly buffer in_dir_lights {
	DirLight dir_lights[];
};
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

struct PointLight {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
};
layout(binding=3) readonly buffer in_point_lights {
	PointLight point_lights[];
};
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos);

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

void main(
	void
) {
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	vec3 result = vec3(0.);
	for (int i = 0; i < n_dir_lights; ++i) {
		result += CalcDirLight(dir_lights[i], norm, viewDir);
	}
	for (int i = 0; i < n_point_lights; ++i) {
		result += CalcPointLight(point_lights[i], norm, viewDir, FragPos);
	}

	FragColor = vec4(result, 1.);
}

vec3
CalcDirLight(
	DirLight light,
	vec3 normal,
	vec3 viewDir
) {
	vec3 lightDir = normalize(-light.direction);
	// sample texture for diffuse/ambient
	vec3 sampled_diffuse  = vec3(texture(sampler2D(diffuseTexture , boxSampler), TexCoord));
	vec3 sampled_specular = vec3(texture(sampler2D(specularTexture, boxSampler), TexCoord));
	// ambient
	vec3 ambient = light.ambient * sampled_diffuse;
	// diffuse
	float diff = max(dot(normal, lightDir), 0.);
	vec3 diffuse = light.diffuse * diff * sampled_diffuse;
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.), mat_shininess);
	vec3 specular = light.specular * spec * sampled_specular;
	return ambient + diffuse + specular;
}

vec3
CalcPointLight(
	PointLight light,
	vec3 normal,
	vec3 viewDir,
	vec3 fragPos
) {
	DirLight equiv_dir_light;
	equiv_dir_light.direction = fragPos - light.position, // expect normalization in `CalcDirLight()`
	equiv_dir_light.ambient   = light.ambient;
	equiv_dir_light.diffuse   = light.diffuse;
	equiv_dir_light.specular  = light.specular;

	vec3 result = CalcDirLight(equiv_dir_light, normal, viewDir);

	// attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1. / (light.constant + light.linear * distance + light.quadratic * distance * distance);

	return attenuation * result;
}
@end

@program scene scene_vs scene_fs

