

#version 330 core
out vec4 color;
in vec3 vertex_pos;
in vec2 vertex_tex;
in vec3 vertex_normal;


uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;

uniform vec3 camoff;
uniform vec3 campos;
uniform vec3 lightpos;
uniform vec3 playerPos;



void main()
{
	float ambient = .5;
	vec3 texcolor = vec3(.6,.5,.4);

	vec2 texcoords=vertex_tex;
	float t= 1./100.;
	texcoords -= vec2(camoff.x,camoff.z)*t;

	//vec3 heightcolor = texture(tex4, texcoords).rgb;
	//heightcolor.r = 0.1 + heightcolor.r*0.9;
	//color.rgb = texture(tex2, texcoords*50).rgb * heightcolor.r;
	

	texcolor = texture(tex2, texcoords*50).rgb;
	
	//color.rgb *= vertex_pos.y/5;

	vec3 n = normalize(vertex_normal);
	vec3 lp = lightpos;
	vec3 ld = normalize(lp-vertex_pos);

	vec3 pd = playerPos - vertex_pos;
	float pdist = length(pd);
	if (4.0f >= pdist)
	{
		ambient *= (pdist/4.0f);
	}
	color.rgb = texcolor*ambient;

	float angWidth = atan(2.0f,pdist);
	float angle = acos(dot(normalize(pd),ld));
	float shadow = 1.0f;
	if (angle < angWidth)
	{
		shadow = 1 - angle/angWidth;
		shadow *= clamp(10/pdist,0,1);
		shadow = 1-shadow;
		//shadow = clamp(shadow,0,1);
	}
	
	float diffuse = dot(n,ld);
	//diffuse = clamp(diffuse,-.25,1);
	diffuse*=2;
	color.rgb += texcolor*diffuse*shadow;

	


	color.a=1;

	float len = length(vertex_pos.xz+campos.xz);
	len-=41;
	len/=8.;
	len=clamp(len,0,1);
	color.a=1-len;


}
