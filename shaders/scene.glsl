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
layout(binding=0) uniform texture2D diffuseTexture;
layout(binding=1) uniform texture2D specularTexture;
layout(binding=0) uniform sampler boxSampler; // there only needs to be one of these for many
                                              // material texture lookups
layout(binding=1) uniform scene_fs_params {
	vec3 viewPos;
};
layout(binding=2) uniform scene_material {
	float mat_shininess;
};
layout(binding=3) uniform scene_light {
	vec3  light_position;
	vec3  light_direction;
	vec3  light_ambient;
	vec3  light_diffuse;
	vec3  light_specular;
	float light_cutoff;
	float light_outer_cutoff;
};

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

void main(
	void
) {
	// sample texture for diffuse/ambient
	vec3 sampled_diffuse  = vec3(texture(sampler2D(diffuseTexture , boxSampler), TexCoord));
	vec3 sampled_specular = vec3(texture(sampler2D(specularTexture, boxSampler), TexCoord));

	// spotlight
	vec3 lightDir = normalize(light_position - FragPos);
	float cos_theta = dot(lightDir, normalize(-light_direction));
	float epsilon = light_cutoff - light_outer_cutoff;
	float intensity = smoothstep(0.f, 1.f, (cos_theta - light_outer_cutoff) / epsilon);

	// ambient
	vec3 ambient = light_ambient * sampled_diffuse;
	ambient *= intensity;

	// diffuse
	vec3 norm = normalize(Normal);
	float diff = max(dot(norm, lightDir), 0.);
	vec3 diffuse = light_diffuse * diff * sampled_diffuse;
	diffuse *= intensity;

	// specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.), mat_shininess);
	vec3 specular = light_specular * spec * sampled_specular;

	vec3 result = ambient + diffuse + specular;
	FragColor = vec4(result, 1.);
}
@end

@program scene scene_vs scene_fs

