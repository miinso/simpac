#include <Eigen/Dense>
// #include <array>
#include <vector>

Eigen::Matrix<float, 3, 2> inline triangle_deformation_gradient(const Eigen::Vector3f &x0,
                                                                const Eigen::Vector3f &x1,
                                                                const Eigen::Vector3f &x2,
                                                                const Eigen::Matrix2f &dm_inv)
{
	// func
	Eigen::Matrix<float, 3, 2> ds;
	ds.col(0) = x1 - x0;
	ds.col(1) = x2 - x0;

	return ds * dm_inv;
}

void inline eval_stretch_kernel(const std::vector<Eigen::Vector3f> &pos_list,
                                const Eigen::VectorXf              &face_area_list,
                                const std::vector<Eigen::Matrix2f> &dm_inv_list,
                                const std::vector<Eigen::Vector3i> &faces,
                                const std::vector<Eigen::Vector3f> &aniso_ke,
                                // outputs
                                std::vector<Eigen::Vector3f> &forces)
{
    // kernel
	auto fid = 0;        // should be.. <thread id | cuda kernel id | flecs iter>

	const auto &dm_inv    = dm_inv_list[fid];
	auto        face_area = face_area_list[fid];
	const auto &face      = faces[fid];

	const auto &x0 = pos_list[face.x()];
	const auto &x1 = pos_list[face.y()];
	const auto &x2 = pos_list[face.z()];

	auto F       = triangle_deformation_gradient(x0, x1, x2, dm_inv);
	auto Fu_norm = F.col(0).norm();
	auto Fv_norm = F.col(1).norm();
	// NOTE: use `.squaredNorm` maybe?
	auto Fu = Fu_norm > 1e-6f ? F.col(0).normalized() : Eigen::Vector3f::Zero();
	auto Fv = Fv_norm > 1e-6f ? F.col(1).normalized() : Eigen::Vector3f::Zero();

	const auto dFu_dx = Eigen::Vector3f(-dm_inv(0, 0) - dm_inv(1, 0), dm_inv(0, 0), dm_inv(1, 0));
	const auto dFv_dx = Eigen::Vector3f(-dm_inv(0, 1) - dm_inv(1, 1), dm_inv(0, 1), dm_inv(1, 1));

	const auto ku = aniso_ke[fid][0];        // `u` direction (weft) stiffness
	const auto kv = aniso_ke[fid][1];        // `v` direction (warp) stiffness
	const auto ks = aniso_ke[fid][2];        // shear stiffness

	for (int i = 0; i < 3; ++i)
	{
		const auto force = -face_area * (ku * (Fu_norm - 1.0f) * dFu_dx[i] * Fu +
		                                 kv * (Fv_norm - 1.0f) * dFv_dx[i] * Fv +
		                                 ks * Fu.dot(Fv) * (Fu * dFv_dx[i] + Fv * dFu_dx[i]));

		forces[face[i]] += force;
	}
}

// NaN? (c or cpp11)
//
// Eigen::Vector3f::Constant(NAN);
// std::numeric_limits<float>::quiet_NaN();
// auto huh = Eigen::Vector3f(1, NAN, 2);
