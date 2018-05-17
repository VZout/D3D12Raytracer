#pragma once

#include <array>
#include <cstdint>

#include "../rt_structs.hlsl"
#include "../raytracer.ps"

template<std::uint32_t N>
class BVH
{
public:
	BVH() = default;
	~BVH() = default;

	void Construct(std::vector<Vertex> const& scene_vertices, std::vector<std::uint16_t> const& scene_indices)
	{
		for (std::uint32_t i = 0; i < N; i++)
		{
			m_indices[i] = i;
		}

		node_pool_ptr = 0; // 2 because allignment I believe.

		auto root = node_pool[0];
		root.left_first = 0;
		root.count = N;
		Subdivide(root, scene_vertices, scene_indices/*, root.left_first, root.count*/);
	}

private:
	inline BBox CalculateBoundingBox(std::vector<Vertex> const& scene_vertices, std::vector<std::uint16_t> const& scene_indices)
	{
		float min_x = std::numeric_limits<float>::infinity();
		float max_x = std::numeric_limits<float>::lowest();
		float min_y = std::numeric_limits<float>::infinity();
		float max_y = std::numeric_limits<float>::lowest();
		float min_z = std::numeric_limits<float>::infinity();
		float max_z = std::numeric_limits<float>::lowest();
		for (auto idx : scene_indices)
		{
			if (scene_vertices[idx].position.x < min_x) min_x = scene_vertices[idx].position.x;
			if (scene_vertices[idx].position.x > max_x) max_x = scene_vertices[idx].position.x;
			if (scene_vertices[idx].position.y < min_y) min_y = scene_vertices[idx].position.y;
			if (scene_vertices[idx].position.y > max_y) max_y = scene_vertices[idx].position.y;
			if (scene_vertices[idx].position.z < min_z) min_z = scene_vertices[idx].position.z;
			if (scene_vertices[idx].position.z > max_z) max_z = scene_vertices[idx].position.z;
		}

		BBox retval;

		retval.left = min_x;
		retval.right = max_x;
		retval.top = max_y;
		retval.bottom = min_y;
		retval.front = min_z;
		retval.back = max_z;
		return retval;
	}

	inline std::pair<std::vector<std::uint16_t>, std::vector<std::uint16_t>> Partition(BVHNode const& node, std::vector<Vertex> const& scene_vertices, std::vector<std::uint16_t> const& scene_indices)
	{
		auto bb = node.bbox;
		float split_plane_x = bb.left + (bb.right - bb.left) * 0.5; // TODO correct?
		split_plane_x = 0;

		std::vector<std::uint16_t> left_partition;
		std::vector<std::uint16_t> right_partition;

		for (auto i = 0; i < scene_indices.size(); i += 3)
		{
			auto a = scene_vertices[scene_indices[i]];
			auto b = scene_vertices[scene_indices[i+1]];
			auto c = scene_vertices[scene_indices[i+2]];

			if (a.position.x < split_plane_x && b.position.x < split_plane_x && c.position.x < split_plane_x)
			{
				left_partition.push_back(scene_indices[i]);
				left_partition.push_back(scene_indices[i + 1]);
				left_partition.push_back(scene_indices[i + 2]);
			}
			else // If the triangle doesn't fully fit in the left plane just put it in the right no matter how much its overlapping.
			{
				right_partition.push_back(scene_indices[i]);
				right_partition.push_back(scene_indices[i + 1]);
				right_partition.push_back(scene_indices[i + 2]);
			}
		}

		return { left_partition, right_partition };
	}

	int q = 0;

	inline void Subdivide(BVHNode target, std::vector<Vertex> const& scene_vertices, std::vector<std::uint16_t> const& scene_indices /*, std::uint32_t first, std::uint32_t count*/)
	{
		target.bbox = CalculateBoundingBox(scene_vertices, scene_indices);

		target.left_first = node_pool_ptr += 2;
		auto p = Partition(target, scene_vertices, scene_indices);

		// Stop subdividing if we have less than 3 primitives to subdivide. Also stop subdividing if a subdivide failed and one of the partitions is empty.
		if (p.first.empty() || p.second.empty() || target.count < 3)
		{
			if (q == 0)
			{
				target.count_triangles = scene_indices.size() / 3;
				for (auto i : scene_indices)
				{
					target.bib_start = big_index_buffer.size() - 1;
					big_index_buffer.push_back(i);
				}
			}
			q++;
			return;
		}
		else
		{
			target.count_triangles = 0;
			target.bib_start = 0;
		}

		node_pool[target.left_first].count = p.first.size() / 3;
		node_pool[target.left_first + 1].count = p.second.size() / 3;
		Subdivide(node_pool[target.left_first], scene_vertices, p.first);
		Subdivide(node_pool[target.left_first + 1], scene_vertices, p.second);
	}

	std::array<std::uint16_t, N> m_indices;
	std::array<BVHNode, N * 2 - 1> node_pool;
	std::uint32_t node_pool_ptr;
public:
	std::vector<std::uint16_t> big_index_buffer;
};