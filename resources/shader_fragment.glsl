#version 330 core
out vec4 color;
in vec3 vertex_normal;
in vec3 vertex_pos;
in vec2 vertex_tex;
in float ambient;
uniform vec3 basecolor;
uniform vec3 campos;
uniform vec3 playerPos;
uniform float shiny;
uniform int lightingType;
uniform int objType;

uniform vec3 lightpos;
uniform vec3 scale;

uniform sampler2D tex0;
//uniform sampler2D tex1;
//uniform sampler2D tex2;
//uniform sampler2D tex3;
//uniform sampler2D tex4;
//uniform sampler2D tex5;


vec4 lightColor = vec4(1,1,1,1);

void main()
{
	vec3 n = normalize(vertex_normal);
	vec3 lp = lightpos;
	vec3 ld = normalize(lp-vertex_pos);
	//float lightdist = length(lp - vertex_pos);
	float diffuse = dot(n,ld);
	diffuse = clamp(diffuse,0,1);
	vec3 pd = playerPos - vertex_pos;
	float pdist = length(pd);
	

	

	vec4 texcolor = lightColor/2;
	color = vec4(0,0,0,0);
	float shadow = 1.0f;

	


	

	if (1 == objType) //wall
	{
		vec2 scaled_tex;
		if (abs(n.y) > max(abs(n.x),abs(n.z)))
		{
			scaled_tex.x = vertex_tex.x*scale.x;
			scaled_tex.y = vertex_tex.y*scale.z;
		}
		else 
		{
			scaled_tex.x = vertex_tex.x*scale.x;
			scaled_tex.y = vertex_tex.y*scale.y;
		}
		scaled_tex/=5;
		
		texcolor = texture(tex0, scaled_tex);

		float angWidth = atan(2.0f,pdist);
		float angle = acos(dot(normalize(pd),ld));
		if (angle < angWidth)
		{
			shadow = 1 - angle/angWidth;
			shadow *= clamp(10/pdist,0,1);
			shadow = 1-shadow;
			//shadow = clamp(shadow,0,1);
		}
	}
	else
	{
		texcolor = texture(tex0, vertex_tex);
	}
	texcolor*=vec4(basecolor,1.0);

	color = texcolor;//*basecolor;

	if (1 <= lightingType ) //ambient
	{
		float ambShadow = 1.0f;
		if (4.0f >= pdist && objType !=2)
		{
			ambShadow = (pdist/4.0f);
		}
		color = texcolor*ambient*ambShadow;
	}
	
	if (2 <= lightingType) //diffuse
	{
		

		color += texcolor*lightColor*diffuse*shadow;
		//color/=2;
	}

	if (3 <= lightingType) //specular
	{
		vec3 cd = normalize(campos - vertex_pos);
		vec3 h = normalize(cd+ld);
		float spec = dot(n,h);
		spec = clamp(spec,0,1);
		spec = pow(spec,shiny);
		color += vec4(vec3(1,1,1)*spec*3*clamp(shiny,0,1)*diffuse*shadow, 0);	 
	}

}
