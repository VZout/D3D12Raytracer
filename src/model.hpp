#pragma once

#include <string>
#include <vector>
#include <map>
#include <assimp/Importer.hpp>

#include "vec.hpp"
#include "bone.hpp"
#include "skeleton.hpp"

namespace rlr
{

	struct Vertex
	{
		fm::vec3 m_pos;
		fm::vec3 m_normal;
		fm::vec2 m_texCoord;
		fm::vec3 m_tangent;
		fm::vec3 m_bitangent;
		float weight[4] = { 0, 0, 0, 0 };
		uint32_t id[4] = { 0, 0, 0, 0 };
	};

	struct Animation;

	static int WEIGHTS_PER_VERTEX = 4;

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		Skeleton skeleton;
	};

	struct Model
	{
		Model();
		~Model();
		Model(const Model& rhs);

		int id_offset = 0;

		std::vector<Mesh> meshes;
		std::vector<Bone> bones;
		std::vector<Animation*> animations;
		aiMatrix4x4 global_invere_transform;
		std::vector<aiNode*> ai_nodes;
		std::vector<aiNodeAnim*> ai_nodes_anim;
		std::string directory;
		Assimp::Importer importer;
	};

	void Load(Model& model, std::string const & path);

} /* rlr */
