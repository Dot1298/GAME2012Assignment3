#define PAR_SHAPES_IMPLEMENTATION
#define FAST_OBJ_IMPLEMENTATION
#include <par_shapes.h>
#include <fast_obj.h>
#include "Mesh.h"
#include <cassert>
#include <cstdio>

void Upload(Mesh* mesh);

void CreateMesh(Mesh* mesh, const char* path)
{
	fastObjMesh* obj = fast_obj_read(path); //Load OBJ file using fast_obj and store mesh data in obj
	int count = obj->index_count; //Get count on indices from OBJ file (rep num of triangles)
	mesh->positions.resize(count); //Resize mesh's positions to hold vertex data
	mesh->normals.resize(count);  //Resize mesh's normals

	assert(obj->position_count > 1); //Obj file asserted to have more than 1 position
	for (int i = 0; i < count; i++) 
	{
		// Using the obj file's indices, populate the mesh->positions with the object's vertex positions
		fastObjIndex idx = obj->indices[i]; //Used to access the correct vertex data based on index

		mesh->positions[i] = {
			obj->positions[idx.p * 3 + 0],   //x
			obj->positions[idx.p * 3 + 1],   //y
			obj->positions[idx.p * 3 + 2]    //z
		};
	}
	
	assert(obj->normal_count > 1);
	for (int i = 0; i < count; i++)
	{
		// Using the obj file's indices, populate the mesh->normals with the object's vertex normals
		fastObjIndex idx = obj->indices[i];

		mesh->normals[i] = {
			obj->normals[idx.n * 3 + 0],   //x
			obj->normals[idx.n * 3 + 1],   //y
			obj->normals[idx.n * 3 + 2]    //z
		};
	}
	
	if (obj->texcoord_count > 1)
	{
		mesh->tcoords.resize(count);
		for (int i = 0; i < count; i++)
		{
			// Using the obj file's indices, populate the mesh->tcoords with the object's vertex texture coordinates
			fastObjIndex idx = obj->indices[i];

			mesh->tcoords[i] = {
				obj->texcoords[idx.t * 2 + 0], //x
				obj->texcoords[idx.t * 2 + 1], //y
			};
		}
	}
	else
	{
		printf("**Warning: mesh %s loaded without texture coordinates**\n", path);
	}
	fast_obj_destroy(obj);
	mesh->count = count;

	Upload(mesh);
}

void CreateMesh(Mesh* mesh, ShapeType shape)
{
	// 1. Generate par_shapes_mesh
	par_shapes_mesh* par = nullptr;
	switch (shape)
	{
	case HEAD:
		par = par_shapes_create_plane(1, 1);
		break;

	case CUBE:
		par = par_shapes_create_cube();
		break;

	case SPHERE:
		par = par_shapes_create_subdivided_sphere(1);
		break;

	default:
		assert(false, "Invalid shape type");
		break;
	}
	
	par_shapes_compute_normals(par);

	// 2. Convert par_shapes_mesh to our Mesh representation
	int count = par->ntriangles * 3;	// 3 points per triangle
	mesh->count = count;
	mesh->indices.resize(count);
	memcpy(mesh->indices.data(), par->triangles, count * sizeof(uint16_t));
	mesh->positions.resize(par->npoints);
	memcpy(mesh->positions.data(), par->points, par->npoints * sizeof(Vector3));
	mesh->normals.resize(par->npoints);
	memcpy(mesh->normals.data(), par->normals, par->npoints * sizeof(Vector3));
	if (par->tcoords != nullptr)
	{
		mesh->tcoords.resize(par->npoints);
		memcpy(mesh->tcoords.data(), par->tcoords, par->npoints * sizeof(Vector2));
	}
	else
	{
		printf("Warning: Mesh generated without tcoords!\n");
	}
	par_shapes_free_mesh(par);

	// 3. Upload Mesh to GPU
	Upload(mesh);
}

void DestroyMesh(Mesh* mesh)
{

}

void DrawMesh(const Mesh& mesh)
{
	glBindVertexArray(mesh.vao);
	if (mesh.ebo != GL_NONE)
		glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_SHORT, nullptr);
	else
		glDrawArrays(GL_TRIANGLES, 0, mesh.count);
	glBindVertexArray(GL_NONE);
}

void Upload(Mesh* mesh)
{
	GLuint vao, pbo, nbo, tbo, ebo;
	vao = pbo = nbo = tbo = ebo = GL_NONE;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_ARRAY_BUFFER, pbo);
	glBufferData(GL_ARRAY_BUFFER, mesh->positions.size() * sizeof(Vector3), mesh->positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &nbo);
	glBindBuffer(GL_ARRAY_BUFFER, nbo);
	glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(Vector3), mesh->normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
	glEnableVertexAttribArray(1);

	if (!mesh->tcoords.empty())
	{
		glGenBuffers(1, &tbo);
		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->tcoords.size() * sizeof(Vector2), mesh->tcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), nullptr);
		glEnableVertexAttribArray(2);
	}
	
	if (!mesh->indices.empty())
	{
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(uint16_t), mesh->indices.data(), GL_STATIC_DRAW);
	}

	glBindVertexArray(GL_NONE);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);

	mesh->vao = vao;
	mesh->pbo = pbo;
	mesh->nbo = nbo;
	mesh->tbo = tbo;
	mesh->ebo = ebo;
}
