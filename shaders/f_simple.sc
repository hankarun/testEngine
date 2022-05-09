$input v_pos, v_view, v_normal, v_color0


#include <bgfx_shader.sh>
#include "shaderlib.sh"

void main() {
	vec4 color = v_color0;

	vec3 lightPos = vec3(0.0, -10.0, 0.0);
	vec3 normal = normalize(v_normal);
	vec3 view = normalize(v_view);
	vec3 lightDir = normalize(lightPos - v_pos);  
	float ndotl = dot(normal, lightDir);
	float l = saturate(ndotl);

	color.xyz = toLinear(color.xyz) * l;
	gl_FragColor = toGamma(color);
}
