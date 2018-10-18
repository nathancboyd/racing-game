

#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 vertex_pos;
out vec2 vertex_tex;
out vec3 vertex_normal;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;

uniform vec3 camoff;

float hashf(float n) { return fract(sin(n) * 753.5453123); }
float snoise(vec3 x)
{
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0f - 2.0f * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	float result = mix(mix(mix(hashf(n + 0.0), hashf(n + 1.0), f.x),
		mix(hashf(n + 157.0), hashf(n + 158.0), f.x), f.y),
		mix(mix(hashf(n + 113.0), hashf(n + 114.0), f.x),
			mix(hashf(n + 270.0), hashf(n + 271.0), f.x), f.y), f.z);
	return result / 1.0f;
}

float terrainHeight(vec3 v)
{
	return (8 + sin(v.x) + sin(v.x * 3.14159) + sin(v.x / 2.71818) + sin(v.z) + sin(v.z / 3.14159) + sin(v.z * 2.71818) + 2.0*snoise(v)) / 16;
}

void main()
{
	vec3 v, v1, v2, v3, v4, v5, v6;
	v = v1 = v2 = v3 = v4 = v5 = v6 = vertPos;
	v1.z+=1;
	v2.z+=1; 
	v2.x+=1;
	v3.x+=1;
	v4.z-=1;
	v5.z-=1; 
	v5.x-=1;
	v6.x-=1;

	 v.y = terrainHeight(v);
	v1.y = terrainHeight(v1);
	v2.y = terrainHeight(v2);
	v3.y = terrainHeight(v3);
	v4.y = terrainHeight(v4);
	v5.y = terrainHeight(v5);
	v6.y = terrainHeight(v6);

	float a1 = dot(normalize(v1-v),normalize(v2-v));
	float a2 = dot(normalize(v2-v),normalize(v3-v));
	float a3 = dot(normalize(v3-v),normalize(v4-v));
	float a4 = dot(normalize(v4-v),normalize(v5-v));
	float a5 = dot(normalize(v5-v),normalize(v6-v));
	float a6 = dot(normalize(v6-v),normalize(v1-v));

	vec3 fn1 = cross(v1-v,v2-v);
	vec3 fn2 = cross(v2-v,v3-v);
	vec3 fn3 = cross(v3-v,v4-v);
	vec3 fn4 = cross(v4-v,v5-v);
	vec3 fn5 = cross(v5-v,v6-v);
	vec3 fn6 = cross(v6-v,v1-v);


	//vertex_normal = (fn1+fn2+fn3+fn4+fn5+fn6);
	//vertex_normal = v;
	vertex_normal = normalize(a1*fn1+a2*fn2+a3*fn3+a4*fn4+a5*fn5+a6*fn6);
	



	//vec2 texcoords=vertTex;
	//float t = 1./100.;
	//texcoords -= vec2(camoff.x,camoff.z)*t;
	//float height = texture(tex4, texcoords).r;
	
	//height *= 10.0-5;
	//height=0;


	vec4 tpos =  vec4(v, 1.0);
	//tpos.y = terrainHeight(vec3(tpos));
	//tpos.z -=camoff.z;
	//tpos.x -=camoff.x;

	tpos =  M * tpos;


	//tpos.y += height;
	//tpos.y*=753;

	vertex_pos = tpos.xyz;

	gl_Position = P * V * tpos;
	vertex_tex = vertTex;
}
