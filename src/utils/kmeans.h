#pragma once

namespace smp::utils::kmeans
{
	struct PointData
	{
		PointData() = default;
		PointData(const std::vector<uint8_t>& values, uint32_t pixel_count);

		std::vector<uint8_t> values;
		uint32_t pixel_count = 0;
	};

	struct ClusterData
	{
		std::vector<uint8_t> central_values;
		std::vector<const PointData*> points;
	};

	std::vector<ClusterData> run(const std::vector<PointData>& points, uint32_t K, uint32_t max_iterations);
}
