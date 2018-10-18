/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
shared_ptr<Shape> justice;
//shared_ptr<Shape> flyingcar;
//shared_ptr<Shape> wraith;
shared_ptr<Shape> sphere;
shared_ptr<Shape> cube;
shared_ptr<Shape> tetrahedron;
shared_ptr<Shape> cylinder;
shared_ptr<Shape> plasma;
shared_ptr<Shape> myship;
shared_ptr<Shape> myship2;

#define MESHSIZE 100
#define TRACKSCALE 20.0f
#define SHIPSCALE 2.0f
#define GRAV .5737

/*Thrust, Drag, TurnRate*/
vec3 TYPE_ZERO(.75, .333, .9);
vec3 TYPE_ONE(1.3, .77, .7);

static const float PI = 3.1415926f;


double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}

int sgn(float f)
{
	return ((0 < f) - (f < 0));
}

void printVector(glm::vec3 v)
{
	printf("%4.5f %4.5f %4.5f\n",v.x,v.y,v.z);
}

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

//Outputs range from 0 to 1
float terrainHeight(vec3 v)
{
	//return  (1 + sin(v.x * 1) + sin(.789*v.z * 1) + 1.0f*snoise(v));
	return (8 + sin(v.x) + sin(v.x * 3.14159) + sin(v.x / 2.71818) + sin(v.z) + sin(v.z / 3.14159) + sin(v.z * 2.71818) + 2.0*snoise(v)) / 16;
}

class projectile
{
public:
	glm::vec3 scale;
	glm::vec3 vel;
	float grav;
	glm::vec3 pos; 
	vec3 accel;

	projectile(vec3 position, vec3 dir)
	{
		vel = 5.0f*dir;
		pos = position;
		scale = glm::vec3(2.0f);
		accel;
	}

	glm::mat4 process(double ftime, glm::vec3 collision)
	{
		float depth = length(collision);
		accel = vec3(0.0f, -GRAV*ftime, 0.0f);


		if (0.0f != depth)
		{
			glm::vec3 collisionNormal = glm::normalize(collision);
			glm::vec3 velNormal = glm::normalize(vel);
			glm::mat4 T = glm::translate(glm::mat4(1), collisionNormal*depth*1.5f);
			pos = vec3(T*vec4(pos, 1.0f));
			float dp = dot(velNormal, collisionNormal);
			glm::vec3 reflect = velNormal - 2.0f*dp*collisionNormal;

			reflect = normalize(reflect);

			//vel = length(vel) * reflect;
			
			//vel += length(vel) *dp * collisionNormal*1.0f;
			accel.y += reflect.y *GRAV* 1 * ftime;
		}
		//printVector(vel);
		
		vel += accel;
		pos += vel;
		//printVector(pos);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		glm::mat4 S = glm::scale(glm::mat4(1), scale);
		return T * S;
	}

};

int projCount=0;
int maxProj = 25;
vector<projectile> projectiles;

vector<double> framevec;

class playerModel
{
public:
	glm::vec3 pos, rot, scale, vel, accel, rvel, dir;

	vector<vec3> collisionPoints;
	vector <vec3> targets;
	int w, a, s, d, space, z, up, down, left, right, q, e, r, b, lshift;
	int type, currentTar, targetCount;
	float drag, thrust, speed, jump, airbrake, boost, turnrate;
	float tSinceFired, tLastLap, tCurrentLap, tBestLap;
	
	playerModel()
	{
		type = 0;
		w = a = s = d = space = z = up = down = left = right = q = e = r = b = lshift = 0;
		pos = glm::vec3(10 * TRACKSCALE, 5*SHIPSCALE, 15 * TRACKSCALE);
		rot = glm::vec3(0, -3.14159/2, 0);
		dir = rot;
		scale = vec3(SHIPSCALE);
		vel = glm::vec3(0, 0, 0);
		accel = glm::vec3(0, 0, 0);
		airbrake = 5;//	
		speed = 0;
		jump = 40;
		turnrate = .67;
		collisionPoints.push_back(pos);
		collisionPoints.push_back(pos+dir*2.0f);
		tSinceFired = tLastLap = tCurrentLap = tBestLap = 0;
		currentTar = 2;
		targets.push_back(vec3(12, 1, 15)*TRACKSCALE);
		targets.push_back(vec3(25, 1, 15 )*TRACKSCALE);
		targets.push_back(vec3(19 , 1, 11)*TRACKSCALE);
		targetCount = targets.size();
	}

	void collide(vec3 posOffset, vec3 newVel)
	{
		pos += posOffset;
		vel = newVel;
	}

	glm::mat4 process(double ftime, glm::vec3 collision)
	{
		

		tSinceFired = min(tSinceFired + (float)ftime, 1.0f);
		framevec.push_back(ftime);
		double time=0;
		while (framevec.size() > 30)
		{
			framevec.erase(framevec.begin());
		}
		for (double t : framevec)
		{
			time += t;
		}
		time /= framevec.size();
		printf("FPS: %2.1f (%3.0f ms)\n", 1/time, time * 1000);
		
		vec3 target = targets[currentTar];
		vec3 dif = vec3(pos.x, 0.0, pos.z) - vec3(target.x, 0.0, target.z);
		if (target.y > length(dif))
		{
			currentTar = (currentTar + 1) % targetCount;
			//cout << currentTar;
			//cout << "\n";
			if (currentTar == 1)
			{
				tLastLap = tCurrentLap;
				if (tLastLap < tBestLap || tBestLap == 0)
				{
					tBestLap = tLastLap;
				}
				tCurrentLap = 0;
			}
		}
		printf("Current: %3.3f Last: %3.3f Best: %3.3f\n", tCurrentLap, tLastLap, tBestLap);

		tCurrentLap += ftime;


		float depth = length(collision);
		if (1 == type)
		{
			thrust = 2*TYPE_ONE[0];
			drag = TYPE_ONE[1];
			turnrate = TYPE_ONE[2];
		}
		else
		{
			thrust = 2*TYPE_ZERO[0];
			drag = TYPE_ZERO[1];
			turnrate = TYPE_ZERO[2];
		}
		boost = 1;
		

		float yangle = 0;
		float xangle = 0;
		float zangle = 0;

		accel = { 0, -GRAV * ftime, 0 };
		//vel = glm::vec3(0, 0, 0);

		//float speed = 0;
		//float strafe = 0;
		//float vert;

		
		if (lshift == 1)
		{
			boost = 1.5;
		}
		if (w == 1)
		{
			accel.z += -thrust*boost * ftime;
		}
		else if (s == 1)
		{
			accel.z += .5*thrust/boost * ftime;
		}
		if (space == 1)
		{
			drag *= .5*(2-type)*airbrake;
			//yangle += .1*ftime;
		}


		if (z == 1)
		{
			accel.y += -thrust * ftime;
		}
		else if (b == 1 && (pos.y <=0 || collision.y>0))
		{
			accel.y += jump * ftime;
		}
		if (r == 1)
		{
			//reset
			pos = { 10.0*TRACKSCALE, 5.0*SHIPSCALE, 15 * TRACKSCALE };
			rot = { 0, -3.14159 / 2, 0 };
			depth = 0.0f;
			vel = { 0, 0, 0 };
			currentTar = 2;
			tCurrentLap = 0;
		}
		if (1 == up)
		{
			type = 1;
		}
		else if (1 == down)
		{
			type = 0;
		}

		
		if (a == 1)
		{
			accel.x += -.7*thrust/boost * ftime;
			accel.z *= 0.7;//keep lateral speed from exceeding forward speed
		}
		else if (d == 1)
		{
			accel.x += .7*thrust/boost * ftime;
			accel.z *= 0.7;
		}
		if (left == 1)
		{
			yangle = 1 * ftime;
			zangle = 0.5*ftime;
		}
		else if (right == 1)
		{
			yangle = -1 * ftime;
			zangle = -0.5*ftime;
		}

		if (pos.y <= 0)
		{
			pos.y = 0;
			float  dp = dot(normalize(vel), vec3(0, 1, 0));
			vel.y = abs(vel.y);
			vel += length(vel)*dp*0.9f;
			accel.y += 1.879 * ftime;
		}

		

		if (0.0f != depth)
		{
			glm::vec3 collisionNormal = glm::normalize(collision);
			glm::vec3 velNormal = glm::normalize(vel);
			glm::mat4 T = glm::translate(glm::mat4(1), collisionNormal*depth*1.5f);
			pos = vec3(T*vec4(pos,1.0f));
			float dp = dot(velNormal, collisionNormal);
			glm::vec3 reflect = velNormal - 2.0f*dp*collisionNormal;
			
			reflect = normalize(reflect);

			vel = length(vel) * reflect;
			vel += length(vel) *dp * collisionNormal*0.9f;
			
			//float f = sqrt(reflect.x*reflect.x + reflect.z*reflect.z);
			if (collisionNormal.y <= sqrt(collisionNormal.x*collisionNormal.x + collisionNormal.z*collisionNormal.z))
			{
				
				//cout << "hit wall\n";
				accel.z -= thrust*speed*sgn(accel.z)*ftime; 
				accel.x -= thrust*speed*sgn(accel.x)*ftime;
				//vel.x = 0;
				//vel.z = 0;
			}
			else
			{
				//cout << "hit floor\n";
				accel.y += reflect.y * 1.879* ftime;
			}
			
			//accel.y += GRAV * ftime;
			
		}


		rot.y += yangle/(boost*boost)*turnrate;
		//x and z rot cause gimbal lock
		//rot.x += xangle;
		//zangle *= .99;
		rot.z += zangle/(boost);
		//rot.z = min(rot.z, .999f*3.14159f/4.0f);

		//rot.z *= abs(rot.z);
		rot.z *= .9;
		glm::mat4 Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
		glm::mat4 Ry2 = glm::rotate(glm::mat4(1), -rot.y, glm::vec3(0, 1, 0));
		mat4 S = glm::scale(glm::mat4(1.0f), scale);


		
		vel += vec3(vec4(accel, 0.0)*Ry2);
		vel -= normalize(vel)*drag * length(vel)*length(vel)*(float)ftime; //drag force
		//vel *= .97; //fake drag
		speed = sqrt(vel.x*vel.x+vel.z*vel.z);
		//printf("Speed: %2.3f\n", length(vel));

		printf("MPH: %4.0f\n",length(vel) /time * 4 *60 * 60 / 1000 *.6214);
		//printf("g force: %2.2f\n", abs(accel.z) / time * 4 / 9.81);
		//printf("vertical g: %2.2f\n", abs(accel.y) / time * 4 / 9.81);
		//printf("lateral g: %2.2f\n", abs(accel.x) / time * 4 / 9.81);
		
		rvel = vec3(Ry2*vec4(vel, 0.0));
		dir = normalize(vec3(Ry*vec4(0.0f, 0.0f, -1.0f, 0.0f)));
		if (e == 1 && .2 < tSinceFired)
		{
			while (projectiles.size()>maxProj)
			{
				projectiles.erase(projectiles.begin());
			}
			//cout << projectiles.size();
			//cout << "\n";
			projectile p = projectile(pos, dir);
			//projectile *p = new projectile(pos, vec3(0.0f, 1.0f, 0.0f));

			projectiles.push_back(p);
			//cout << projCount;
			//cout << "\n";
			projCount = ((projCount + 1) % maxProj);
			tSinceFired = 0;
		}

		/*if (speed > 1.0)
		{
			vel = glm::normalize(vel);
			vel *= 1;
		}*/
		/*if (speed <= .001)
		{
			vel.x = vel.z = 0;
		}*/
		
		//glm::vec3 dir = vec3(vec4(vel, 1.0)*Ry);
		pos += vel;
		collisionPoints[0] = pos;
		collisionPoints[1] = pos + dir * -1.0f;
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		//cout << "Position: ";
		//printVector(pos);
		//cout << "player rot: ";
		//printVector(rot);
		cout << "\n";
		return T*Ry*Rx*Rz*S;
	}
	
};

class enemyModel
{
public:
	glm::vec3 pos, rot, dir, scale, vel, accel;

	float drag, thrust, speed, jump, airbrake, boost, startZoff, turnrate;
	int r = 0;

	vector<vec3> targets; // the y values refer to the SIZE (detection radius) of the target, NOT the position on the y axis. 
	int currentTar = 0;
	int targetCount;
	float s = TRACKSCALE;
	

	enemyModel(float th, float d, float t, float z)
	{
		startZoff = z;
		turnrate = t*2;
		pos = glm::vec3(10 * TRACKSCALE, 5*SHIPSCALE, (15+startZoff) * TRACKSCALE);
		rot = glm::vec3(0, 3.14159/2, 0);
		scale = vec3(SHIPSCALE);
		vel = glm::vec3(0, 0, 0);
		accel = glm::vec3(0, 0, 0);
		airbrake = 5;//	
		speed = 0;
		jump = 40;
		drag = d;//.4;
		thrust = 2*th;// .6836;

		targets.push_back(vec3(28.0f, 1.0f, 14.5f) * s);
		targets.push_back(vec3(33.2f, 1.6f, 11.0f) * s);
		targets.push_back(vec3(30.0f, 1.0f, 6.5f) * s);
		//targets[3] = (vec3(26.0f, 1.0f, 6.5f) *s);
		targets.push_back(vec3(23.0f, 1.0f, 9.0f) * s);
		//targets[5] = (vec3(20.0f, 1.0f, 11.0f) * s);
		targets.push_back(vec3(16.0f, 1.0f, 10.0f) * s);
		targets.push_back(vec3(9.5f, 1.0f, 2.5f) * s);
		targets.push_back(vec3(4.0f, 1.0f, 2.5f) * s);
		targets.push_back(vec3(1.75f, 1.25f, 6.0f) * s);
		targets.push_back(vec3(8.5f, 1.0f, 8.0f) * s);
		//targets[11] = (vec3(10.0f, 1.0f, 10.5f) * s);
		targets.push_back(vec3(9.0f, 1.0f, 12.0f) * s);
		targets.push_back(vec3(12.0f, 1.0f, 15.0f) * s);
		targetCount = targets.size();
	}

	void collide(vec3 posOffset, vec3 newVel)
	{
		pos += posOffset;
		vel = newVel;
	}

	glm::mat4 process(double ftime, glm::vec3 collision)
	{
		float depth = length(collision);
		//cout << "Target: ";
		//cout << currentTar;
		//cout << "\n";

		glm::mat4 Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		vec3 target = targets[currentTar];
		vec3 dif = vec3(pos.x, 0.0, pos.z) - vec3(target.x, 0.0, target.z);
		float tarSize = target.y;
		float tarDist = glm::length(dif);
		vec3 nrdif = normalize(vec3(vec4(dif, 0.0)*Ry));
		float turn = -nrdif.x;
		dir = normalize(vec3(Ry*vec4(0.0f, 0.0f, -1.0f, 0.0f)));

		
		// 1/2*Cd*rho*(m/u)^3
		//.5*0.04*1.225 * 64 = 1.568;
		
		boost = 1;


		float yangle = 0;
		float xangle = 0;
		float zangle = 0;

		if (tarSize > tarDist)
		{
			currentTar = (currentTar + 1) % targetCount;
		}

		accel = { 0, -GRAV * ftime, 0};

		
		if (.087 < abs(turn))
		{
			//cout << "TURNING\n";
			yangle += ftime * sgn(turn);
			zangle -= ftime * sgn(turn);
		}
		vec3 nvel = normalize(vec3(vec4(vel.x,0.0,vel.z, 0.0)*Ry));

		if (.087 > abs(turn) && abs(nvel.x)<.7)
		{
			//cout << "BOOST\n";
			boost = 1.5;
		}

		accel.z += boost * thrust * ftime;

		if (abs(nvel.x)>.7)
		{
			//cout << "SIDE THRUST\n";
			accel.x += .7*thrust / boost * ftime *-sgn(nvel.x);
			accel.z *= 0.7;
			//zangle += ftime * sgn(turn);
		}

		if (r == 1)
		{
			//reset
			pos = { 10 * TRACKSCALE, 5*SHIPSCALE, (15+startZoff) * TRACKSCALE };
			rot = { 0, 3.14159 / 2, 0 };
			vel = { 0, 0, 0 };
			depth = 0.0f;
			currentTar = 0;
		}

		if (pos.y <= 0)
		{
			pos.y = 0;
			float  dp = dot(normalize(vel), vec3(0, 1, 0));
			vel.y = abs(vel.y);
			vel+= length(vel)*dp*vec3(0, 1, 0)*0.9f;
			accel.y += 1.879 * ftime;
		}

		if (0.0f != depth)
		{
			glm::vec3 collisionNormal = glm::normalize(collision);
			glm::vec3 velNormal = glm::normalize(vel);
			glm::mat4 T = glm::translate(glm::mat4(1), collisionNormal*depth*1.5f);
			pos = vec3(T*vec4(pos, 1.0f));
			float dp = dot(velNormal, collisionNormal);
			glm::vec3 reflect = velNormal - 2.0f*dp*collisionNormal;

			reflect = normalize(reflect);

			vel = length(vel) * reflect;
			vel += length(vel) *dp * collisionNormal*0.9f;

			if (collisionNormal.y <= sqrt(collisionNormal.x*collisionNormal.x+ collisionNormal.z*collisionNormal.z))
			{
				accel.z -= thrust * speed*sgn(accel.z)*ftime;
				accel.x -= thrust * speed*sgn(accel.x)*ftime;
			}
			else
			{
				accel.y += reflect.y * 1.879 * ftime;
			}
		}
		rot.y += yangle / boost * turnrate;
		rot.z += zangle;
		rot.z *= .9;
		Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
		glm::mat4 Ry2 = glm::rotate(glm::mat4(1), -rot.y, glm::vec3(0, 1, 0));
		mat4 S = glm::scale(glm::mat4(1.0f), scale);

		vel += vec3(vec4(accel, 0.0)*Ry2);
		vel -= normalize(vel)*drag * length(vel)*length(vel)*(float)ftime; //drag force
																		   //vel *= .97; //fake drag
		speed = sqrt(vel.x*vel.x + vel.z*vel.z);

		if (speed <= .001)
		{
			vel.x = vel.z = 0;
		}

		pos += vel;
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		//cout << "enemy pos: ";
		//printVector(pos);
		//printVector(rot);
		return T * Ry*Rx*Rz*S;
	}
};

class terrainMesh
{
public:
	vec3 vertices[MESHSIZE * MESHSIZE * 4];

	vec3 pos = vec3(0.0f, 1.0f, 0.0f);
	vec3 scale = vec3(7.0f, 5.0f, 7.0f);

	glm::mat4 RotateTerrain = glm::rotate(glm::mat4(1.0f), 3.14159f / 2.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 TranslateTerrain = glm::translate(glm::mat4(1.0f), pos);
	glm::mat4 ScaleTerrain = glm::scale(glm::mat4(1.0f), scale);

	glm::mat4 TranslateTerrain2 = glm::translate(glm::mat4(1.0f), -pos);
	glm::mat4 ScaleTerrain2 = glm::scale(glm::mat4(1.0f), 1.0f/scale);

	terrainMesh()
	{
		
	}
	mat4 getModelMatrix()
	{
		return TranslateTerrain * ScaleTerrain;
	}
	vec3 collisionCheck(vec3 objPos)
	{
		float forcefield = 2.0f;//1.0f *SHIPSCALE;
		vec3 collisionNormal = vec3(0.0, 0.0, 0.0);
		vec3 objPos2 = vec3(TranslateTerrain2*ScaleTerrain2*vec4(objPos, 1.0));
		objPos2.y = 0;
		//printVector(objPos2);

		//if (objPos.x<(pos.x )*scale.x || objPos.z<(pos.z )*scale.z||
		//	objPos.x>(pos.x + MESHSIZE )*scale.x || objPos.z>(pos.z + MESHSIZE )*scale.z  )
		if (objPos2.x<0 || objPos2.z<0||
			objPos2.x>MESHSIZE || objPos2.z>MESHSIZE  )
		{
			return collisionNormal;
		}

		if (objPos.y > (pos.y+scale.y+forcefield))
		{
			return collisionNormal;
		}

		//cout << "over terrain\n";

		int xint = floor(objPos2.x);
		int zint = floor(objPos2.z);
		float xfract = fract(objPos2.x);
		float zfract = fract(objPos2.z);
		//cout << xfract;
		//cout << "\n";

		vec3 v1 = vertices[xint * 4 + zint * MESHSIZE * 4 + 0];
		vec3 v2;
		vec3 v3;

		if (xfract > zfract)
		{
			v2 = vertices[xint * 4 + zint * MESHSIZE * 4 + 1];
			v3 = vertices[xint * 4 + zint * MESHSIZE * 4 + 2];
		}
		else
		{
			v2 = vertices[xint * 4 + zint * MESHSIZE * 4 + 2];
			v3 = vertices[xint * 4 + zint * MESHSIZE * 4 + 3];
		}
		//printVector(v1);
		//v1 = vec3(TranslateTerrain*ScaleTerrain*vec4(v1, 1.0)*);
		//v2 = vec3(vec4(v2, 1.0)*TranslateTerrain);
		//v3 = vec3(vec4(v3, 1.0)*TranslateTerrain);

		//v1 = vec3(vec4(v1, 1.0)*);
		//v2 = vec3(vec4(v2, 1.0)*ScaleTerrain);
		//v3 = vec3(vec4(v3, 1.0)*ScaleTerrain);

		float Area = glm::length(glm::cross(v1 - v2, v1 - v3)) / 2;
		//cout << Area;
		//cout << "\n";
		float Area1 = glm::length(glm::cross(objPos2 - v2, objPos2 - v3)) / 2;
		float Area2 = glm::length(glm::cross(objPos2 - v3, objPos2 - v1)) / 2;
		float Area3 = glm::length(glm::cross(objPos2 - v1, objPos2 - v2)) / 2;
		/*cout << Area1;
		cout << " ";
		cout << Area2;
		cout << " ";
		cout << Area3;
		cout << "\n";*/

		//float yx = v3.y*xfract + v1.y*(1-xfract);
		
		v1.y = terrainHeight(v1);
		v2.y = terrainHeight(v2);
		v3.y = terrainHeight(v3);
		

		float height = (v1.y*Area1 + v2.y * Area2 + v3.y * Area3) / (Area);

	

		v1 = vec3(getModelMatrix() * vec4(v1, 1.0));
		v2 = vec3(getModelMatrix() * vec4(v2, 1.0));
		v3 = vec3(getModelMatrix() * vec4(v3, 1.0));



		height *= scale.y;
		height += forcefield;
		//printVector(v1);


		vec3 faceNormal = normalize(glm::cross(v1 - v2, v1 - v3));
		//faceNormal.y *= -1;
		//cout << "faceNormal ";
		//printVector(faceNormal);


		
		//float height = (v1.y + v2.y + v3.y) / 3;
		//cout << "height:";
		//cout << height;

		//cout << " pos.y:";
		//cout << objPos.y;
		float depth = (height - objPos.y ) * dot(vec3(0.0, 1.0, 0.0), faceNormal);
		//cout << " depth:";
		//cout << depth;

		if (depth < 0)
		{
			collisionNormal = faceNormal * depth;
			//collisionNormal = vec3(0.0, 1.0, 0.0) * (height - objPos.y);
			//collisionNormal = vec3(vec4(collisionNormal, 1.0)*TranslateTerrain*ScaleTerrain);
			//cout << " collision";
		}
		//cout << "\n";

		return collisionNormal;
	}

};



playerModel player;
enemyModel enemy1 = enemyModel(TYPE_ZERO[0], TYPE_ZERO[1] , TYPE_ZERO[2], .5f);
enemyModel enemy2 = enemyModel(TYPE_ONE[0], TYPE_ONE[1], TYPE_ONE[2], -.5f);
terrainMesh terrain;


class camera
{
public:
	glm::vec3 pos, rot;
	glm::vec3 offset;
	int q;

	camera()
	{
		q = 0;
		pos = rot = glm::vec3(0, 0, 0);
		offset = glm::vec3(0, 4.0, 1.5);

	}
	glm::mat4 process(double ftime, glm::vec3 collision)
	{
		float depth = length(collision);
		pos = player.pos;
		rot = player.rot;

		//rot.y -= player.rot.z*abs(player.rot.z)*10;
		rot.y -= player.rot.z;
		
		//offset = glm::vec3(0, .75 - player.speed / 4, 4 - player.speed);//justice
		offset = glm::vec3(0, .75 - player.speed / 5, 3 - (abs(player.rvel.x) + player.rvel.z)/2);//myship
		offset.x = .5*player.rvel.x;
		offset.x *= abs(offset.x) *-1;

		offset.y -= clamp(player.rvel.y*abs(player.rvel.y)*0.5f,-0.5f,0.5f);

		//cout << "offset: ";
		//printVector(offset);

		if (1 == q)
		{
			offset = vec3(0, .75, 4);
			rot.y = player.rot.y + 3.14159f;
		}

		glm::mat4 Ry = glm::rotate(glm::mat4(1), -rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Ry2 = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		//glm::mat4 Rx = glm::rotate(glm::mat4(1), -player.rot.x, glm::vec3(1, 0, 0));
		//glm::mat4 Rz = glm::rotate(glm::mat4(1), -player.rot.z, glm::vec3(0, 0, 1));

		mat4 S = glm::scale(glm::mat4(1.0f), 1.0f/player.scale);
		mat4 S2 = glm::scale(glm::mat4(1.0f), player.scale);
		
		pos += vec3(Ry2*S2*vec4(offset, 1.0f));
		if (0.0f != depth)
		{
			vec3 collisionNormal = normalize(collision);
			float move = 1.5f*abs(depth/dot(collisionNormal, normalize(offset)));
			//offset *= move / length(offset);
			offset.z -= move;
		}
		offset.z = max(offset.z, 0.0f);

		glm::mat4 T = glm::translate(glm::mat4(1), -player.pos);
		glm::mat4 T2 = glm::translate(glm::mat4(1), -offset);
		
		

		//pos = vec3(T*T2*vec4(pos, 1.0f));
		//pos *= -1;

		return  T2 * S * Ry * T;
	}
};

camera mycam;

vec3 vehicleCollisionCheck(vec3 playerpos, vec3 enemypos)
{
	vec3 result = vec3(0, 0, 0);
	vec3 diff =  playerpos - enemypos;
	float dist = abs(length(diff));
	float depth = dist - 1.0f*SHIPSCALE;
	if (depth > 0 || depth< -1)
	{
		return result;
	}
	/*cout << "COLLISION\n";
	cout << dist;
	cout << "\n";
	cout << depth;
	cout << "\n";*/
	//printVector(playerpos);
	//printVector(enemypos);
	//return result;
    return (depth)* normalize(diff);
}



class Wall
{
public:
	glm::vec3 pos;
	glm::vec3 scale;
	glm::vec3 rot;
	float spaceDiagonal; // the long axis
	float forcefield = 2.0;// *SHIPSCALE;//1.0f; //

	glm::mat4 T;
	glm::mat4 S;//hitbox scale
	glm::mat4 S2;//model scale
	glm::mat4 R , Rx, Ry, Rz;

	Wall(glm::vec3 p, glm::vec3 s, glm::vec3 r)
	{
		pos = p;
		scale = s;
		rot = r;

		Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
		//R = glm::rotate(glm::mat4(1), -3.1415926f / 2.0f, rot);
		T = glm::translate(glm::mat4(1.0f), pos);
		S = glm::scale(glm::mat4(1.0f), scale + forcefield);

		spaceDiagonal = glm::length(scale + forcefield);
	}
	glm::mat4 getModelMatrix()
	{
		S2 = glm::scale(glm::mat4(1.0f), scale);
		return T * Rx*Ry*Rz*S2;;
	}
	glm::vec3 collisionCheck(glm::vec3 objPos, int isCam)
	{
	    vec3 dist = pos - objPos;
		if (isCam)
		{
			forcefield = 0.5f;
		}
		else
		{
			forcefield = 1.5f;
		}

		if (glm::length(dist) > spaceDiagonal)
        {
            //std::cout << "NOT in collision sphere\n";
            return { 0,0,0 };
        }
	    //std::cout << "in collision sphere\n";
        glm::mat4 Ry2 = glm::rotate(glm::mat4(1), -rot.y, glm::vec3(0, 1, 0));
        glm::mat4 Rx2 = glm::rotate(glm::mat4(1), -rot.x, glm::vec3(1, 0, 0));
        glm::mat4 Rz2 = glm::rotate(glm::mat4(1), -rot.z, glm::vec3(0, 0, 1));
        //glm::mat4 R2 = glm::rotate(glm::mat4(1), -3.1415926f / 2.0f, vec3(rot.x, rot.y, rot.z));
        dist = vec3(vec4(dist, 0.0)*Rx*Ry*Rz);
    
        glm::vec3 depth;
        depth.x = abs(dist.x) - (scale + forcefield).x;
        depth.y = abs(dist.y) - (scale + forcefield).y;
        depth.z = abs(dist.z) - (scale + forcefield).z;
	    if (depth.x >= 0 || depth.y>=0 || depth.z>=0)
            return { 0,0,0 };

        float m = max(depth.y, max(depth.z, depth.x));
        
        glm::vec3 normal = { 0,0,0 };

        if (m == depth.x)
        {
            //cout << "x ";
            m *= sgn(dist.x);
            normal = glm::vec3(glm::vec4(m, 0.0f, 0.0f, 0.0));
            
        }
        else if (m == depth.y)
        {
            //cout << "y ";
            m *= sgn(dist.y);
            normal = glm::vec3(glm::vec4( 0.0f, m, 0.0f, 0.0));
        }
        else if (m == depth.z)
        {
            //cout << "z ";
            m *= sgn(dist.z);
            normal = glm::vec3(glm::vec4(0.0f, 0.0f, m, 0.0));
        }
        normal = vec3(vec4(normal, 0.0)*Rx2*Ry2*Rz2);
        
        return (normal);// , m);

	
	}
};

Wall createWall(vec3 p1, vec3 p2)
{
	vec3 diff = p1 - p2;
	vec3 rot;
	//rot.x = dot(normalize(vec2(diff.y, diff.z)), normalize(vec2(p2.y, p2.z)));
	//rot.y = acos(dot(normalize(vec2(diff.x, diff.z)), vec2(1, 0)) / length(vec2( diff.x, diff.z )));
	//rot.z = dot(normalize(vec2(diff.x, diff.y)), normalize(vec2(p2.x, p2.y)));
	rot.x = rot.z = 0.0f;
	rot.y = -atan2(diff.z,diff.x);

	return Wall((p1+p2)/2.0f, vec3(length(diff)/2.0f, .75*TRACKSCALE, 0.1*TRACKSCALE), rot );
}

vector<Wall> walls;






class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog, heightshader;


	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexColorIDBox, IndexBufferIDBox, InstanceBuffer;
	GLuint MeshPosID, MeshTexID, NorBufID;
	GLuint VertexTexBox, VertexNormDBox;

	//texture data
	GLuint Texture0;
	GLuint Texture1;
	GLuint Texture2;
	GLuint Texture3;
	GLuint Texture4;
	GLuint Texture5;
	GLuint Texture6;
	GLuint Texture7;
	GLuint Texture8;


	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			player.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			player.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			player.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			player.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			player.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			player.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			player.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			player.d = 0;
		}

		if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		{
			player.z = 1;
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
		{
			player.z = 0;
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			player.space = 1;
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			player.space = 0;
		}


		if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		{
			player.up = 1;
		}
		if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
		{
			player.up = 0;
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		{
			player.down = 1;
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
		{
			player.down = 0;
		}

		if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		{
			player.left = 1;
		}
		if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
		{
			player.left = 0;
		}
		if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		{
			player.right = 1;
		}
		if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
		{
			player.right = 0;
		}

		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		{
			mycam.q = 1;
		}
		if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
		{
			mycam.q = 0;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
		{
			player.e = 1;
		}
		if (key == GLFW_KEY_E && action == GLFW_RELEASE)
		{
			player.e = 0;
		}
		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			player.r = 1;
			enemy1.r = 1;
			enemy2.r = 1;
		}
		if (key == GLFW_KEY_R && action == GLFW_RELEASE)
		{
			player.r = 0;
			enemy1.r = 0;
			enemy2.r = 0;
		}
		if (key == GLFW_KEY_B && action == GLFW_PRESS)
		{
			player.b = 1;
		}
		if (key == GLFW_KEY_B && action == GLFW_RELEASE)
		{
			player.b = 0;
		}
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
		{
			player.lshift = 1;
		}
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
		{
			player.lshift = 0;
		}


	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		return;
		double posX, posY;
		float newPt[2];
		if (action == GLFW_PRESS)
		{
			glfwGetCursorPos(window, &posX, &posY);
			std::cout << "Pos X " << posX <<  " Pos Y " << posY << std::endl;

			//change this to be the points converted to WORLD
			//THIS IS BROKEN< YOU GET TO FIX IT - yay!
			newPt[0] = 0;
			newPt[1] = 0;

			std::cout << "converted:" << newPt[0] << " " << newPt[1] << std::endl;
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
			//update the vertex array with the updated points
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*6, sizeof(float)*2, newPt);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}

	void init_mesh()
	{
		float scale = 1;
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);
		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &MeshPosID);
		glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);

		for (int x = 0; x<MESHSIZE; x++)
			for (int z = 0; z < MESHSIZE; z++)
			{
				terrain.vertices[x * 4 + z * MESHSIZE * 4 + 0] = (vec3(0.0, 0.0, 0.0) + vec3(x, 0, z))*scale;
				terrain.vertices[x * 4 + z * MESHSIZE * 4 + 1] = (vec3(1.0, 0.0, 0.0) + vec3(x, 0, z))*scale;
				terrain.vertices[x * 4 + z * MESHSIZE * 4 + 2] = (vec3(1.0, 0.0, 1.0) + vec3(x, 0, z))*scale;
				terrain.vertices[x * 4 + z * MESHSIZE * 4 + 3] = (vec3(0.0, 0.0, 1.0) + vec3(x, 0, z))*scale;
			}
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * MESHSIZE * MESHSIZE * 4, terrain.vertices, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//tex coords
		float t = 1. / 100;
		//scale = 1;
		vec2 tex[MESHSIZE * MESHSIZE * 4];
		for (int x = 0; x<MESHSIZE; x++)
			for (int y = 0; y < MESHSIZE; y++)
			{
				tex[x * 4 + y * MESHSIZE * 4 + 0] = (vec2(0.0, 0.0) + vec2(x, y)*t)*scale;
				tex[x * 4 + y * MESHSIZE * 4 + 1] = (vec2(t, 0.0) + vec2(x, y)*t)*scale;
				tex[x * 4 + y * MESHSIZE * 4 + 2] = (vec2(t, t) + vec2(x, y)*t)*scale;
				tex[x * 4 + y * MESHSIZE * 4 + 3] = (vec2(0.0, t) + vec2(x, y)*t)*scale;
			}
		glGenBuffers(1, &MeshTexID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, MeshTexID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * MESHSIZE * MESHSIZE * 4, tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &IndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		GLushort elements[MESHSIZE * MESHSIZE * 6];
		int ind = 0;
		for (int i = 0; i<MESHSIZE * MESHSIZE * 6; i += 6, ind += 4)
		{
			elements[i + 0] = ind + 0;
			elements[i + 1] = ind + 1;
			elements[i + 2] = ind + 2;
			elements[i + 3] = ind + 0;
			elements[i + 4] = ind + 2;
			elements[i + 5] = ind + 3;
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*MESHSIZE * MESHSIZE * 6, elements, GL_STATIC_DRAW);
		glBindVertexArray(0);
	}

	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
		//generate the VAO
		
		init_mesh();
		
		string resourceDirectory = "../resources" ;
		// Initialize mesh.
		myship = make_shared<Shape>();
		myship->loadMesh(resourceDirectory + "/myship1-flat-tex.obj");
		myship->resize();
		myship->init();

		myship2 = make_shared<Shape>();
		myship2->loadMesh(resourceDirectory + "/myship2-flat-tex.obj");
		myship2->resize();
		myship2->init();

		justice = make_shared<Shape>();
		justice->loadMesh(resourceDirectory + "/justice.obj");
		justice->resize();
		justice->init();

		/*flyingcar = make_shared<Shape>();
		flyingcar->loadMesh(resourceDirectory + "/flyingcar.obj");
		flyingcar->resize();
		flyingcar->init();*/

		/*wraith = make_shared<Shape>();
		wraith->loadMesh(resourceDirectory + "/wraith.obj");
		wraith->resize();
		wraith->init();*/

		sphere = make_shared<Shape>();
		sphere->loadMesh(resourceDirectory + "/sphere.obj");
		sphere->resize();
		sphere->init();

		cube = make_shared<Shape>();
		cube->loadMesh(resourceDirectory + "/cube.obj");
		cube->resize();
		cube->init();
		
		tetrahedron = make_shared<Shape>();
		tetrahedron->loadMesh(resourceDirectory + "/tetrahedron.obj");
		tetrahedron->resize();
		tetrahedron->init();

		cylinder = make_shared<Shape>();
		cylinder->loadMesh(resourceDirectory + "/cylinder.obj");
		cylinder->resize();
		cylinder->init();

		plasma = make_shared<Shape>();
		plasma->loadMesh(resourceDirectory + "/plasma.obj");
		plasma->resize();
		plasma->init();



		int width, height, channels;
		char filepath[1000];

		//texture 1
		string str = resourceDirectory + "/metal.jpg";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 2
		str = resourceDirectory + "/sky.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 3
		str = resourceDirectory + "/dirt.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, Texture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/concrete.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, Texture3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/plasma.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, Texture4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/holo.png";//"/holo.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, Texture5);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/alpha.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, Texture6);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/myship1 tex blur.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture7);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, Texture7);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		str = resourceDirectory + "/myship2 tex.png";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture8);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, Texture8);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex0Location = glGetUniformLocation(prog->pid, "tex0");//tex, tex2... sampler in the fragment shader
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex1");
		GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
		GLuint Tex3Location = glGetUniformLocation(prog->pid, "tex3");
		GLuint Tex4Location = glGetUniformLocation(prog->pid, "tex4");
		GLuint Tex5Location = glGetUniformLocation(prog->pid, "tex5");

		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex0Location, 0);
		/*glUniform1i(Tex1Location, 1);
		glUniform1i(Tex2Location, 2);
		glUniform1i(Tex3Location, 3);
		glUniform1i(Tex4Location, 4);
		glUniform1i(Tex5Location, 5);*/


		Tex0Location = glGetUniformLocation(heightshader->pid, "tex0");//tex, tex2... sampler in the fragment shader
		Tex1Location = glGetUniformLocation(heightshader->pid, "tex1");
		Tex2Location = glGetUniformLocation(heightshader->pid, "tex2");
		Tex3Location = glGetUniformLocation(heightshader->pid, "tex3");
		Tex4Location = glGetUniformLocation(heightshader->pid, "tex4");
		Tex5Location = glGetUniformLocation(heightshader->pid, "tex5");
		// Then bind the uniform samplers to texture units
		glUseProgram(heightshader->pid);
		glUniform1i(Tex0Location, 0);
		glUniform1i(Tex1Location, 1);
		glUniform1i(Tex2Location, 2);
		glUniform1i(Tex3Location, 3);
		glUniform1i(Tex4Location, 4);
		glUniform1i(Tex5Location, 5);


		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	}



	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();
		
		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		// Initialize the GLSL program.
		prog = std::make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
		prog->addUniform("shiny");

		//0 no lighting
		//1 ambient only
		//2 ambient+diffuse
		//3 ambient+diffuse+specular
		prog->addUniform("lightingType");
		prog->addUniform("objType");//3 hologram, 2=ship, 1=wall, 0=other
		prog->addUniform("basecolor");
		prog->addUniform("lightpos");
		prog->addUniform("scale");
		prog->addUniform("playerPos");

		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");

		
		// Initialize the GLSL program.
		heightshader = std::make_shared<Program>();
		heightshader->setVerbose(true);
		heightshader->setShaderNames(resourceDirectory + "/height_vertex.glsl", resourceDirectory + "/height_frag.glsl");
		if (!heightshader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		heightshader->addUniform("P");
		heightshader->addUniform("V");
		heightshader->addUniform("M");
		heightshader->addUniform("camoff");
		heightshader->addUniform("campos");
		heightshader->addUniform("lightpos");
		heightshader->addUniform("playerPos");

		heightshader->addAttribute("vertPos");
		heightshader->addAttribute("vertTex");


		vector<vec3> points;
		vector<vec3> points2;

		points.push_back(vec3(8, 0, 16));
		points.push_back(vec3(31, 0, 16));
		points.push_back(vec3(34, 0, 15));
		points.push_back(vec3(35, 0, 11));
		points.push_back(vec3(34, 0, 7));
		points.push_back(vec3(29, 0, 5));
		points.push_back(vec3(24, 0, 5));
		points.push_back(vec3(22, 0, 9));
		points.push_back(vec3(19, 0, 10));
		points.push_back(vec3(15, 0, 8));
		points.push_back(vec3(13, 0, 4));
		points.push_back(vec3(11, 0, 1));
		points.push_back(vec3(6, 0, 0));
		points.push_back(vec3(0, 0, 4));
		points.push_back(vec3(0, 0, 9));
		points.push_back(vec3(3, 0, 11));
		points.push_back(vec3(8, 0, 9));
		points.push_back(vec3(9, 0, 10));
		points.push_back(vec3(7, 0, 14));

		points2.push_back(vec3(12, 0, 14));
		points2.push_back(vec3(28, 0, 14));
		points2.push_back(vec3(31, 0, 12));
		points2.push_back(vec3(31, 0, 9));
		points2.push_back(vec3(28, 0, 7));
		points2.push_back(vec3(25, 0, 8));
		points2.push_back(vec3(23, 0, 11));
		points2.push_back(vec3(19, 0, 12));
		points2.push_back(vec3(15, 0, 11));
		points2.push_back(vec3(11, 0, 5));
		points2.push_back(vec3(9, 0, 3));
		points2.push_back(vec3(5, 0, 3));
		points2.push_back(vec3(3, 0, 6));
		points2.push_back(vec3(5, 0, 7));
		points2.push_back(vec3(10, 0, 6));
		points2.push_back(vec3(12, 0, 10));
		points2.push_back(vec3(10, 0, 13));


		/*walls.push_back(Wall(glm::vec3(0.0f, 0.0f, 120.0f), glm::vec3(100.0f, 20.0f, 20.0f), glm::vec3(3.1416f/-8.0f, 0.0f, 0.0f)));
		walls.push_back(Wall(glm::vec3(120.0f, 0.0f, 40.0f), glm::vec3(20.0f, 20.0f, 60.0f), glm::vec3(0.0, 0.0f, 0.0f)));
		walls.push_back(Wall(glm::vec3(60.0f, 0.0f, -60.0f), glm::vec3(40.0f, 20.0f, 40.0f), glm::vec3(0.0, 0.0f, 0.0f)));
		walls.push_back(Wall(glm::vec3(-40.0f, 0.0f, -120.0f), glm::vec3(60.0f, 20.0f, 20.0f), glm::vec3(0.0, 0.0f, 0.0f)));
		walls.push_back(Wall(glm::vec3(-120.0f, 0.0f, 0.0f), glm::vec3(20.0f, 20.0f, 100.0f), glm::vec3(0.0, 0.0f, 0.0f)));
		walls.push_back(Wall(glm::vec3(-40.0f, 0.0f, 0), glm::vec3(20.0f, 20.0f, 60.0f), glm::vec3(0.0, 0.0f, 0.0f)));
		walls.push_back(Wall(glm::vec3(20.0f, 0.0f, 40.0f), glm::vec3(40.0f, 20.0f, 20.0f), glm::vec3(0.0, 0.0f, 0.0f)));
		walls.push_back(Wall(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(200.0f, 2.0f, 200.0f), glm::vec3(3.14f*-0.02f, 0.0f, 0.0f)));//floor
		walls.push_back(Wall(glm::vec3(0.0f, 3.0f, -40.0f), glm::vec3(20.0f, 2.0f, 20.0f), glm::vec3(3.14f*+0.02f, 0.0f, 0.0f)));*/
		
		float s = TRACKSCALE;
		for (int i=0;i<points.size()-1; i++)
		{
			walls.push_back(createWall(points[i]* s, points[i+1]* s));
		}
		walls.push_back(createWall(points[points.size()-1] * s, points[0] * s));

		s = TRACKSCALE;
		for (int i = 0; i<points2.size() - 1; i++)
		{
			walls.push_back(createWall(points2[i] * s, points2[i + 1] * s));
		}
		walls.push_back(createWall(points2[points2.size() - 1] * s, points2[0] * s));
		
		walls.push_back(Wall(glm::vec3(20.0f*s, 3.0f, 15.0f*s), glm::vec3(1.5f*s, 1.0f, 1.0f*s), glm::vec3( 0.0f, 0.0f, 3.14f*0.06f)));//jump
		walls.push_back(Wall(glm::vec3(20.0f*s, 1.0f, 10.0f*s), glm::vec3(35.0f*s, 1.0f, 20.0f*s), glm::vec3(3.14f*0.01f, 0.0f,0.0f)));//floor

		walls.push_back(Wall(glm::vec3(32.5f*s, 1.0f, 11.0f*s), glm::vec3(2.5f*s, 1.0f, 5.0f*s), glm::vec3(0.0f, 0.0f, 3.14f*0.05f)));
		walls.push_back(Wall(glm::vec3(3.75f*s, 3.5f, 6.25f*s), glm::vec3(3.75f*s, 1.0f, 5.0f*s), glm::vec3(0.0f, 0.0f, 3.14f*-0.04f)));
		walls.push_back(Wall(glm::vec3(20.0f*s, 1.0f, 9.25f*s), glm::vec3(6.0f*s, 1.0f, 3.75f*s), glm::vec3(3.14f*-0.05f, 0.0f,  0.0f)));
	}


	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
		double frametime = get_last_elapsed_time();

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now

		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = glm::mat4(1);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);
		if (width < height)
		{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -2.0f, 100.0f);
		}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones

		//animation with the model matrix:
		static float w = 0.0;
		w += 1.0 * frametime;//rotation angle
		float trans = 0;// sin(t) * 2;
		glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), w / 2, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 RotateY2 = glm::rotate(glm::mat4(1.0f), w / 3, glm::vec3(0.0f, -1.0f, 0.0f));
		float angle = -3.1415926 / 2.0;
		glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 RotateX2 = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 1.0f, 0.0f));
		glm::mat4 RotateY3 = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, -1.0f));
		glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
		glm::mat4 TransX = glm::translate(glm::mat4(1.0f), glm::vec3(.5f, 0.0f, 0.5f));
		glm::mat4 TransX2 = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, 0.0f));
		glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));
		glm::mat4 S3 = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
		glm::mat4 T2 = glm::translate(glm::mat4(1), vec3(0.0f, -0.1f, 0.4f));


		prog->bind();


		//player.process(frametime);

		vec3 lightpos = glm::vec3(350.0f + 350.0f*sin(w / 3), 100, 200.0f + 200.0f*cos(w / 3));
		glUniform3fv(prog->getUniform("lightpos"), 1, &lightpos[0]);
		glUniform3fv(prog->getUniform("scale"), 1, &vec3(1.0, 1.0, 1.0)[0]);
		vec3 playerCollision = { 0.0f,0.0f,0.0f };
		vec3 enemy1Collision = { 0.0f,0.0f,0.0f };
		vec3 enemy2Collision = { 0.0f,0.0f,0.0f };
		vec3 cameraCollision = { 0.0f,0.0f,0.0f };

		for (Wall w : walls)
		{
			playerCollision += w.collisionCheck(player.collisionPoints[0], 0);

			playerCollision += w.collisionCheck(player.collisionPoints[1], 0);

			enemy1Collision += w.collisionCheck(enemy1.pos, 0);
			enemy2Collision += w.collisionCheck(enemy2.pos, 0);
			cameraCollision += w.collisionCheck(mycam.pos, 1);
		}
		playerCollision += terrain.collisionCheck(player.pos);
		enemy1Collision += terrain.collisionCheck(enemy1.pos);
		enemy2Collision += terrain.collisionCheck(enemy2.pos);

		vec3 vehicleCollisionP1 = vehicleCollisionCheck(player.pos, enemy1.pos);
		if (vec3(0.0, 0.0, 0.0) != vehicleCollisionP1)
		{
			vec3 totalVel = player.vel + enemy1.vel;
			player.collide(-vehicleCollisionP1, totalVel / 2.0f);
			enemy1.collide(vehicleCollisionP1, totalVel / 2.0f);
		}
		vec3 vehicleCollisionP2 = vehicleCollisionCheck(player.pos, enemy2.pos);
		if (vec3(0.0, 0.0, 0.0) != vehicleCollisionP2)
		{
			vec3 totalVel = player.vel + enemy2.vel;
			player.collide(-vehicleCollisionP2, totalVel / 2.0f);
			enemy2.collide(vehicleCollisionP2, totalVel / 2.0f);
		}
		vec3 vehicleCollision12 = vehicleCollisionCheck(enemy1.pos, enemy2.pos);
		if (vec3(0.0, 0.0, 0.0) != vehicleCollision12)
		{
			vec3 totalVel = enemy1.vel + enemy2.vel;
			enemy1.collide(-vehicleCollision12, totalVel / 2.0f);
			enemy2.collide(vehicleCollision12, totalVel / 2.0f);
		}


		M = player.process(frametime, playerCollision);
		V = mycam.process(frametime, cameraCollision);
		mat4 Vi = glm::transpose(V);
		Vi[0][3] = 0;
		Vi[1][3] = 0;
		Vi[2][3] = 0;

		glm::mat4 TransPlayer = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 RotPlayer = glm::rotate(glm::mat4(1.0f), 2 * angle, glm::vec3(0, 1, 0));
		//M *= TransPlayer * RotPlayer*S3;
		M *= RotPlayer;//* RotateY3;
		glUniform3fv(prog->getUniform("playerPos"), 1, &player.pos[0]);
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(0.5, 0.75, 1)[0]);
		glUniform1f(prog->getUniform("shiny"), 20.0);
		glUniform1i(prog->getUniform("lightingType"), 3);
		glUniform1i(prog->getUniform("objType"), 2);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture0);
		if (player.type)
		{
			//glBindTexture(GL_TEXTURE_2D, Texture8);
			myship2->draw(prog, FALSE);
		}
		else
		{
			//glBindTexture(GL_TEXTURE_2D, Texture7);
			myship->draw(prog, FALSE);
		}
		glBindTexture(GL_TEXTURE_2D, Texture0);
		M = enemy1.process(frametime, enemy1Collision);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(.75, .25, .25)[0]);
		myship->draw(prog, FALSE);

		M = enemy2.process(frametime, enemy2Collision);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(.25, .75, .25)[0]);
		myship2->draw(prog, FALSE);

		glUniform1i(prog->getUniform("objType"), 0);


		//sky sphere
		glm::mat4 ScaleSky = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)*800.0f);
		glm::mat4 TransCamera = glm::translate(glm::mat4(1.0f), player.pos);
		RotateX = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(-1.0f, 0.0f, 0.0f));
		M = TransCamera * RotateX* ScaleSky;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture1);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1, 1, 1)[0]);
		glUniform1i(prog->getUniform("lightingType"), 0);
		sphere->draw(prog, FALSE);



		/*M = TransZ * TransX *  S;
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
		glUniform1f(prog->getUniform("shiny"), 20.0);
		glUniform1i(prog->getUniform("lightingType"), 0);
		shape->draw(prog,FALSE);*/

		//M = TransZ * TransX2 * RotateY2 * RotateX * S;


		glUniform1i(prog->getUniform("lightingType"), 2);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1, 1, 1)[0]);
		glUniform1i(prog->getUniform("objType"), 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture3);
		for (Wall w : walls)
		{
			{

				M = w.getModelMatrix();
				//glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
				//glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
				glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				//glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
				glUniform3fv(prog->getUniform("scale"), 1, &w.scale[0]);
				cube->draw(prog, FALSE);
			}
		}
		glUniform1i(prog->getUniform("objType"), 0);
		glUniform3fv(prog->getUniform("scale"), 1, &vec3(1.0, 1.0, 1.0)[0]);

		M = glm::translate(glm::mat4(1.0f), glm::vec3(380.0f, 20.0f, 165.0f));
		M *= RotateY;
		M *= glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f));
		//M *= glm::rotate(glm::mat4(1.0f), -2 * angle, glm::vec3(0.0f, 0.0f, 1.0f));
		//glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		//glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture0);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(0.4, 0.5, 0.6)[0]);
		//glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
		glUniform1i(prog->getUniform("lightingType"), 3);
		tetrahedron->draw(prog, FALSE);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture0);
		M = glm::translate(glm::mat4(1.0f), glm::vec3(500, 30 + 5 * sin(w / 2), 320));
		M *= glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.5f, 0.0f));
		M *= glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(.9, 1, 1.1)[0]);
		justice->draw(prog, FALSE);


		glBindTexture(GL_TEXTURE_2D, Texture0);
		glUniform1i(prog->getUniform("lightingType"), 3);
		M = glm::translate(glm::mat4(1.0f), vec3(700, 30, 125));
		M *= glm::scale(glm::mat4(1.0f), glm::vec3(25, 25, 25));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(0.4, 0.5, 0.6)[0]);
		sphere->draw(prog, FALSE);


		prog->unbind();
		heightshader->bind();
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glm::mat4 TransY = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		M = terrain.getModelMatrix();
		glUniformMatrix4fv(heightshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniformMatrix4fv(heightshader->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(heightshader->getUniform("V"), 1, GL_FALSE, &V[0][0]);


		vec3 offset = vec3(0.0, 0.0, 0.0);
		//vec3 offset = -mycam.pos;
		offset.y = 0;
		offset.x = (int)offset.x;
		offset.z = (int)offset.z;
		glUniform3fv(heightshader->getUniform("camoff"), 1, &offset[0]);
		glUniform3fv(heightshader->getUniform("campos"), 1, &mycam.pos[0]);
		glUniform3fv(heightshader->getUniform("lightpos"), 1, &lightpos[0]);
		glUniform3fv(heightshader->getUniform("playerPos"), 1, &player.pos[0]);
		glBindVertexArray(VertexArrayID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, Texture4);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, Texture2);
		glDrawElements(GL_TRIANGLES, MESHSIZE*MESHSIZE * 6, GL_UNSIGNED_SHORT, (void*)0);
		heightshader->unbind();

		prog->bind();
		glUniform3fv(prog->getUniform("lightpos"), 1, &lightpos[0]);
		glUniform3fv(prog->getUniform("playerPos"), 1, &player.pos[0]);
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);
		glUniform1f(prog->getUniform("shiny"), 20.0);


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);



		/*glUniform1i(prog->getUniform("lightingType"), 0);
		M = glm::translate(glm::mat4(1.0f), vec3(200, 30, 300));
		M *= glm::scale(glm::mat4(1.0f), glm::vec3(25, 25, 25));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1, .5, 0)[0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture5);
		//glDisable(GL_DEPTH_TEST);
		cube->draw(prog, FALSE);
		//glEnable(GL_DEPTH_TEST);*/

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture6);
		glUniform1i(prog->getUniform("lightingType"), 0);

		
		//the following code is obscene and should not exist
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1, .25, .25)[0]);

		float speed = length(enemy1.vel);
		float n = 20;

		for (float i = 0; i < n; i++)
		{
			M = glm::translate(glm::mat4(1.0f), enemy1.pos + (speed+.5f) * normalize(enemy1.dir*(n - sqrt(i)) - enemy1.vel*sqrt(i))*(1.0f + sqrt(i) *10.0f / n));

			M *= Vi;
			M *= glm::scale(glm::mat4(1.0f), glm::vec3(speed/i));

			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			plasma->draw(prog, FALSE);
		}

		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(.25, .75, .25)[0]);

		speed = length(enemy2.vel);
		for (float i = 0; i < 2*n; i++)
		{
			M = glm::translate(glm::mat4(1.0f), enemy2.pos + (speed+.5f) * normalize(enemy2.dir*(n - sqrt(i)) - enemy2.vel*sqrt(i))*(1.0f + sqrt(i) *10.0f / n));

			M *= Vi;
			M *= glm::scale(glm::mat4(1.0f), glm::vec3(speed / i));
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			plasma->draw(prog, FALSE);
		}

		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(0.6, 0.8, 1)[0]);

		speed = length(player.vel);

		for (float i = 0; i < n; i++)
		{
			M = glm::translate(glm::mat4(1.0f), player.pos - (speed + 1)*normalize(player.dir*(n - sqrt(i)) + player.vel*sqrt(i))*(1.0f + sqrt(i) *10.0f / n));
			M *= Vi;
			M *= glm::scale(glm::mat4(1.0f), glm::vec3(.5*speed*(1 / (i + 1) - 1 / (n + 1))));
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			//glDisable(GL_DEPTH_TEST);
			plasma->draw(prog, FALSE);
			//glEnable(GL_DEPTH_TEST);
		}
	

		

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture5);
		glUniform1i(prog->getUniform("objType"), 3);
		glUniform1i(prog->getUniform("lightingType"), 0);
		M = glm::translate(glm::mat4(1.0f), glm::vec3(25, 40, 60));
		M *= RotateY;
		M *= glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(0.6, 0.8, 1)[0]);
		//glDisable(GL_DEPTH_TEST);
		myship->draw(prog, FALSE);
		//glEnable(GL_DEPTH_TEST);
		

		M = glm::translate(glm::mat4(1.0f), vec3(200, 30, 175));
		M *= RotateY2;
		M *= glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f));
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1, 0.25, 0.25)[0]);
		//glDisable(GL_DEPTH_TEST);
		myship2->draw(prog, FALSE);
		//glEnable(GL_DEPTH_TEST);




		glUniform1i(prog->getUniform("objType"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture4);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1, 1,1)[0]);
		//for (projectile p : projectiles)
		int i = 0;
		int size = projectiles.size();
		for (i = 0; i<size; i++)
		{
			projectile p = projectiles[i];
			vec3 collision = vec3( 0,0,0 );
			for (Wall w : walls)
			{
				collision += w.collisionCheck(p.pos, 0);
			}
			collision += terrain.collisionCheck(p.pos);

			if (collision != vec3(0.0, 0.0, 0.0) &&  (collision.y <= sqrt(collision.x*collision.x+ collision.z*collision.z)))
			{
				projectiles.erase(projectiles.begin() + i);
				i--;
				size = projectiles.size();
			}
			else
			{
				vec3 vCollision1 = vehicleCollisionCheck(p.pos, enemy1.pos);
				vec3 vCollision2 = vehicleCollisionCheck(p.pos, enemy2.pos);
				if (vec3(0.0, 0.0, 0.0) != vCollision1)
				{
					//enemy1.collide(vec3(0.0),vec3(0.0));
					projectiles.erase(projectiles.begin() + i);
					i--;
					size = projectiles.size();
				}
				else if (vec3(0.0, 0.0, 0.0) != vCollision2)
				{
					//enemy2.collide(vec3(0.0), vec3(0.0));
					projectiles.erase(projectiles.begin() + i);
					i--;
					size = projectiles.size();
				}
				else
				{
					M = p.process(frametime, collision);
					M *= Vi;
					projectiles[i] = p;
					
					glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
					//glDisable(GL_DEPTH_TEST);
					plasma->draw(prog, FALSE);	
					//glEnable(GL_DEPTH_TEST);
				}
			}
		}
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);

		/*for (vec3 tar : enemy1.targets)
		{
			M = glm::translate(glm::mat4(1.0f), vec3(tar.x, enemy1.pos.y+2.0*SHIPSCALE, tar.z));
			M *= glm::scale(glm::mat4(1.0f), glm::vec3(tar.y, 1.0f, tar.y));
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(0.4, 0.5, 0.6)[0]);
			glUniform1i(prog->getUniform("objType"), 1);
			cylinder->draw(prog, FALSE);
		}*/
		
		/*for (vec3 cp : player.collisionPoints)
		{
			M = glm::translate(glm::mat4(1.0f), cp);
			M *= glm::scale(glm::mat4(1.0f), glm::vec3(.25, .25, .25));
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			glUniform1i(prog->getUniform("lightingType"), 1);
			glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1.0, 1.0, 1.0)[0]);
			sphere->draw(prog, FALSE);
		}*/



		M = glm::translate(glm::mat4(1.0f), lightpos);
		M *= glm::translate(glm::mat4(1.0f), vec3(0.0f, 5.0f,0.0f));
		M *= glm::rotate(glm::mat4(1.0f), w / 3, glm::vec3(0.0f, 1.0f, 0.0f));
		M *= glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, -1.0f, 0.0f));
		M *= glm::scale(glm::mat4(1.0f), glm::vec3(25, 25, 25));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture0);
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform1i(prog->getUniform("lightingType"), 3);
		glUniform3fv(prog->getUniform("basecolor"), 1, &vec3(1, 1, 1)[0]);
		justice->draw(prog, FALSE);
		prog->unbind();
		
		/*mat4 Vi = glm::transpose(V);
		Vi[0][3] = 0;
		Vi[1][3] = 0;
		Vi[2][3] = 0;
		for (int z = 0; z < 5; z++)
		{
			glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + z, 0.0f, -3 - z));

			M = TransZ * S* Vi;


			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);

		}*/


		/*
		glBindVertexArray(VertexArrayID);
		//actually draw from vertex 0, 3 vertices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
		mat4 Vi = glm::transpose(V);
		Vi[0][3] = 0;
		Vi[1][3] = 0;
		Vi[2][3] = 0;
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, Texture3);
		for (int z = 0; z < 5; z++)
		{
			glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + z, 0.0f, -3 - z));

			M = TransZ * S* Vi;


			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);

		}
		glBindVertexArray(0);

		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
		*/

	}
	
};



//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
