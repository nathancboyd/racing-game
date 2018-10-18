#include "Shape.h"
#include <iostream>

#include "GLSL.h"
#include "Program.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


using namespace std;
using namespace glm;


void Shape::loadMesh(const string &meshName, string *mtlpath, unsigned char *(loadimage)(char const *, int *, int *, int *, int))
{
	// Load geometry
	// Some obj files contain material information.
	// We'll ignore them for this assignment.
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> objMaterials;
	string errStr;
	bool rc = FALSE;
	if (mtlpath)
		rc = tinyobj::LoadObj(shapes, objMaterials, errStr, meshName.c_str(), mtlpath->c_str());
	else
		rc = tinyobj::LoadObj(shapes, objMaterials, errStr, meshName.c_str());


	if (!rc)
	{
		cerr << errStr << endl;
	}
	else if (shapes.size())
	{
		obj_count = shapes.size();
		posBuf = new std::vector<float>[shapes.size()];
		norBuf = new std::vector<float>[shapes.size()];
		texBuf = new std::vector<float>[shapes.size()];
		eleBuf = new std::vector<unsigned int>[shapes.size()];

		eleBufID = new unsigned int[shapes.size()];
		posBufID = new unsigned int[shapes.size()];
		norBufID = new unsigned int[shapes.size()];
		texBufID = new unsigned int[shapes.size()];
		vaoID = new unsigned int[shapes.size()];
		materialIDs = new unsigned int[shapes.size()];

		textureIDs = new unsigned int[shapes.size()];

		for (int i = 0; i < obj_count; i++)
		{
			//load textures
			textureIDs[i] = 0;
			//texture sky			
			posBuf[i] = shapes[i].mesh.positions;
			norBuf[i] = shapes[i].mesh.normals;
			texBuf[i] = shapes[i].mesh.texcoords;
			eleBuf[i] = shapes[i].mesh.indices;
			if (shapes[i].mesh.material_ids.size()>0)
				materialIDs[i] = shapes[i].mesh.material_ids[0];
			else
				materialIDs[i] = -1;

		}
	}
	//material:
	for (int i = 0; i < objMaterials.size(); i++)
		if (objMaterials[i].diffuse_texname.size()>0)
		{
			char filepath[1000];
			int width, height, channels;
			string filename = objMaterials[i].ambient_texname;
			int subdir = filename.rfind("\\");
			if (subdir > 0)
				filename = filename.substr(subdir + 1);
			string str = *mtlpath + filename;
			strcpy(filepath, str.c_str());
			//stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)

			unsigned char* data = loadimage(filepath, &width, &height, &channels, 4);
			glGenTextures(1, &textureIDs[i]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			//delete[] data;
		}
	
	int z;
	z = 0;
}

void Shape::resize()
{
	float minX, minY, minZ;
	float maxX, maxY, maxZ;
	float scaleX, scaleY, scaleZ;
	float shiftX, shiftY, shiftZ;
	float epsilon = 0.001f;

	minX = minY = minZ = 1.1754E+38F;
	maxX = maxY = maxZ = -1.1754E+38F;

	// Go through all vertices to determine min and max of each dimension
	for (int i = 0; i < obj_count; i++)
		for (size_t v = 0; v < posBuf[i].size() / 3; v++)
		{
			if (posBuf[i][3 * v + 0] < minX) minX = posBuf[i][3 * v + 0];
			if (posBuf[i][3 * v + 0] > maxX) maxX = posBuf[i][3 * v + 0];

			if (posBuf[i][3 * v + 1] < minY) minY = posBuf[i][3 * v + 1];
			if (posBuf[i][3 * v + 1] > maxY) maxY = posBuf[i][3 * v + 1];

			if (posBuf[i][3 * v + 2] < minZ) minZ = posBuf[i][3 * v + 2];
			if (posBuf[i][3 * v + 2] > maxZ) maxZ = posBuf[i][3 * v + 2];
		}

	// From min and max compute necessary scale and shift for each dimension
	float maxExtent, xExtent, yExtent, zExtent;
	xExtent = maxX - minX;
	yExtent = maxY - minY;
	zExtent = maxZ - minZ;
	if (xExtent >= yExtent && xExtent >= zExtent)
	{
		maxExtent = xExtent;
	}
	if (yExtent >= xExtent && yExtent >= zExtent)
	{
		maxExtent = yExtent;
	}
	if (zExtent >= xExtent && zExtent >= yExtent)
	{
		maxExtent = zExtent;
	}
	scaleX = 2.0f / maxExtent;
	shiftX = minX + (xExtent / 2.0f);
	scaleY = 2.0f / maxExtent;
	shiftY = minY + (yExtent / 2.0f);
	scaleZ = 2.0f / maxExtent;
	shiftZ = minZ + (zExtent / 2.0f);

	// Go through all verticies shift and scale them
	for (int i = 0; i < obj_count; i++)
		for (size_t v = 0; v < posBuf[i].size() / 3; v++)
		{
			posBuf[i][3 * v + 0] = (posBuf[i][3 * v + 0] - shiftX) * scaleX;
			assert(posBuf[i][3 * v + 0] >= -1.0f - epsilon);
			assert(posBuf[i][3 * v + 0] <= 1.0f + epsilon);
			posBuf[i][3 * v + 1] = (posBuf[i][3 * v + 1] - shiftY) * scaleY;
			assert(posBuf[i][3 * v + 1] >= -1.0f - epsilon);
			assert(posBuf[i][3 * v + 1] <= 1.0f + epsilon);
			posBuf[i][3 * v + 2] = (posBuf[i][3 * v + 2] - shiftZ) * scaleZ;
			assert(posBuf[i][3 * v + 2] >= -1.0f - epsilon);
			assert(posBuf[i][3 * v + 2] <= 1.0f + epsilon);
		}
}


//does not check if the vectors are the same size
std::vector<float> vectorDifference(std::vector<float> v1, std::vector<float> v2)
{
	std::vector<float> result = {0.0f,0.0f,0.0f};
	for (int i = 0; i < 3; i++)
	{
		result[i] = v1[i] - v2[i];
	}
	return result;
}

float vectorLength(std::vector<float> v)
{
	return (float)(sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
}

std::vector<float> crossProduct(std::vector<float> v1, std::vector<float> v2)
{
	std::vector<float> result = { 0.0f,0.0f,0.0f };
	result[0] = v1[1] * v2[2] - v1[2] * v2[1];
	result[1] = v1[2] * v2[0] - v1[0] * v2[2];
	result[2] = v1[0] * v2[1] - v1[1] * v2[0];

	return result;
}

/*std::vector<float> normalize(std::vector<float> v)
{
	float length = vectorLength(v);
	std::vector<float> result = { 0.0f,0.0f,0.0f };
	result[0] = v[0] / length;
	result[1] = v[1] / length;
	result[2] = v[1] / length;

	return result;
}*/


std::vector <float> genVertNormals(std::vector <float> positions, std::vector <unsigned int> elements)
{
	std::vector <float> vertNorBuf (positions.size());

	for (int i = 0; i < elements.size() / 3 - 1; i += 1)
	{
		
		int j1 = elements[3 * i];
		int j2 = elements[3 * i + 1];
		int j3 = elements[3 * i + 2];

		glm::vec3 p1 = { positions[3 * j1 + 0],
						 positions[3 * j1 + 1],
						 positions[3 * j1 + 2] };
		glm::vec3 p2 = { positions[3 * j2 + 0],
						 positions[3 * j2 + 1],
						 positions[3 * j2 + 2] };
		glm::vec3 p3 = { positions[3 * j3 + 0],
						 positions[3 * j3 + 1],
						 positions[3 * j3 + 2] };

		float angle1 = dot(normalize(p2- p1), normalize(p3- p1));
		float angle2 = dot(normalize(p1 - p2), normalize(p3 - p2));
		float angle3 = 3.14159-angle1-angle2;

		//std::vector <float> faceNormal = normalize(crossProduct(vectorDifference(p1, p2), vectorDifference(p3, p2)));
		glm::vec3 faceNormal = glm::normalize(cross(p1 - p2, p3 - p2));

		vertNorBuf[3 * j1 + 0] += angle1 * faceNormal[0];
		vertNorBuf[3 * j1 + 1] += angle1 * faceNormal[1];
		vertNorBuf[3 * j1 + 2] += angle1 * faceNormal[2];

		vertNorBuf[3 * j2 + 0] += angle2 * faceNormal[0];
		vertNorBuf[3 * j2 + 1] += angle2 * faceNormal[1];
		vertNorBuf[3 * j2 + 2] += angle2 * faceNormal[2];

		vertNorBuf[3 * j3 + 0] += angle3 * faceNormal[0];
		vertNorBuf[3 * j3 + 1] += angle3 * faceNormal[1];
		vertNorBuf[3 * j3 + 2] += angle3 * faceNormal[2];

		std::cout << " ";
		std::cout << i;
	}
	for (int i = 0; i < vertNorBuf.size() / 3 - 1; i += 1)
	{
		glm::vec3 vertNormal = normalize(vec3( vertNorBuf[3 * i + 0] , vertNorBuf[3 * i + 1] , vertNorBuf[3 * i + 2] ));
		//std::vector <float> vertNormal = normalize({ vertNorBuf[3 * i + 0] , vertNorBuf[3 * i + 1] , vertNorBuf[3 * i + 2] });
		vertNorBuf[3 * i + 0] += vertNormal[0];
		vertNorBuf[3 * i + 1] += vertNormal[1];
		vertNorBuf[3 * i + 2] += vertNormal[2];

		std::cout << " ";
		std::cout << i;
	}
	std::cout << "\ndone\n";

	return vertNorBuf;
}

/*std::vector <float> genFaceNormals(std::vector <float> positions, std::vector <unsigned int> elements)
{
	std::vector <float> faceNor;

	for (int i = 0; i < elements.size() / 3 - 1; i += 1)
	{

		std::vector <float> p1 = { positions[3 * elements[3 * i] + 0],
			positions[3 * elements[3 * i] + 1],
			positions[3 * elements[3 * i] + 2] };
		std::vector <float> p2 = { positions[3 * elements[3 * i + 1] + 0],
			positions[3 * elements[3 * i + 1] + 1],
			positions[3 * elements[3 * i + 1] + 2] };
		std::vector <float> p3 = { positions[3 * elements[3 * i + 2] + 0],
			positions[3 * elements[3 * i + 2] + 1],
			positions[3 * elements[3 * i + 2] + 2] };


		std::vector <float> normal = normalize(crossProduct(vectorDifference(p1, p2), vectorDifference(p3, p2)));
		faceNor[3 * i + 0] = normal[0];
		faceNor[3 * i + 1] = normal[1];
		faceNor[3 * i + 2] = normal[2];
	}
	return faceNor;
}*/




void Shape::init()
{
	for (int i = 0; i < obj_count; i++)

	{


		// Initialize the vertex array object
		glGenVertexArrays(1, &vaoID[i]);
		glBindVertexArray(vaoID[i]);

		// Send the position array to the GPU
		glGenBuffers(1, &posBufID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, posBufID[i]);
		glBufferData(GL_ARRAY_BUFFER, posBuf[i].size() * sizeof(float), posBuf[i].data(), GL_STATIC_DRAW);

		// Send the normal array to the GPU
		if (norBuf[i].empty())
		{
			norBuf[i] = genVertNormals(posBuf[i], eleBuf[i]);

		//	norBufID[i] = 0;

			
		}
		//else
		{
			glGenBuffers(1, &norBufID[i]);
			glBindBuffer(GL_ARRAY_BUFFER, norBufID[i]);
			glBufferData(GL_ARRAY_BUFFER, norBuf[i].size() * sizeof(float), norBuf[i].data(), GL_STATIC_DRAW);
		}

		// Send the texture array to the GPU
		if (texBuf[i].empty())
		{
			texBufID[i] = 0;
		}
		else
		{
			glGenBuffers(1, &texBufID[i]);
			glBindBuffer(GL_ARRAY_BUFFER, texBufID[i]);
			glBufferData(GL_ARRAY_BUFFER, texBuf[i].size() * sizeof(float), texBuf[i].data(), GL_STATIC_DRAW);
		}

		// Send the element array to the GPU
		glGenBuffers(1, &eleBufID[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf[i].size() * sizeof(unsigned int), eleBuf[i].data(), GL_STATIC_DRAW);

		// Unbind the arrays
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		assert(glGetError() == GL_NO_ERROR);
	}
}





void Shape::draw(const shared_ptr<Program> prog,bool use_extern_texures) const
{
	for (int i = 0; i < obj_count; i++)

	{
		int h_pos, h_nor, h_tex;
		h_pos = h_nor = h_tex = -1;

		glBindVertexArray(vaoID[i]);
		// Bind position buffer
		h_pos = prog->getAttribute("vertPos");
		GLSL::enableVertexAttribArray(h_pos);
		glBindBuffer(GL_ARRAY_BUFFER, posBufID[i]);
		glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

		// Bind normal buffer
		h_nor = prog->getAttribute("vertNor");
		if (h_nor != -1 && norBufID[i] != 0)
		{
			GLSL::enableVertexAttribArray(h_nor);
			glBindBuffer(GL_ARRAY_BUFFER, norBufID[i]);
			glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
		}

		if (texBufID[i] != 0)
		{
			// Bind texcoords buffer
			h_tex = prog->getAttribute("vertTex");
			if (h_tex != -1 && texBufID[i] != 0)
			{
				GLSL::enableVertexAttribArray(h_tex);
				glBindBuffer(GL_ARRAY_BUFFER, texBufID[i]);
				glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
			}
		}

		// Bind element buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID[i]);

		//texture
		
		if (!use_extern_texures)
		{
			int textureindex = materialIDs[i];
			if (textureindex >= 0)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, textureIDs[textureindex]);
			}
		}
		// Draw
		glDrawElements(GL_TRIANGLES, (int)eleBuf[i].size(), GL_UNSIGNED_INT, (const void *)0);

		// Disable and unbind
		if (h_tex != -1)
		{
			GLSL::disableVertexAttribArray(h_tex);
		}
		if (h_nor != -1)
		{
			GLSL::disableVertexAttribArray(h_nor);
		}
		GLSL::disableVertexAttribArray(h_pos);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}