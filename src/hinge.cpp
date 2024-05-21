#include <Eigen/Dense>
#include <iostream>
#include <vector>

using namespace Eigen;
using namespace std;

// XPBD Hinge Constraint
class HingeConstraint
{
public:
	HingeConstraint(int body1, int body2, Vector3d anchor, Vector3d axis)
		: m_body1(body1)
		, m_body2(body2)
		, m_anchor(anchor)
		, m_axis(axis)
	{ }

	void solveConstraint(vector<Vector3d>& positions,
						 vector<Quaterniond>& orientations,
						 double compliance,
						 double dt)
	{
		Vector3d r1 = orientations[m_body1].inverse() * (m_anchor - positions[m_body1]);
		Vector3d r2 = orientations[m_body2].inverse() * (m_anchor - positions[m_body2]);

		Vector3d a1 = orientations[m_body1] * m_axis;
		Vector3d a2 = orientations[m_body2] * m_axis;

		Vector3d n = a1.cross(a2);
		double phi = n.norm();

		if(phi > 1e-6)
		{
			n.normalize();

			Vector3d d = positions[m_body2] + orientations[m_body2] * r2 - positions[m_body1] -
						 orientations[m_body1] * r1;

			double lambda = -phi / (compliance + dt * dt * (1.0 / m1 + 1.0 / m2));

			Vector3d dp1 = -lambda * 0.5 * n;
			Vector3d dp2 = lambda * 0.5 * n;

			Vector3d dq1 = -lambda * 0.5 * r1.cross(n);
			Vector3d dq2 = lambda * 0.5 * r2.cross(n);

			positions[m_body1] += dp1;
			positions[m_body2] += dp2;

			orientations[m_body1] = (orientations[m_body1] *
									 Quaterniond(1.0, 0.5 * dq1.x(), 0.5 * dq1.y(), 0.5 * dq1.z()))
										.normalized();
			orientations[m_body2] = (orientations[m_body2] *
									 Quaterniond(1.0, 0.5 * dq2.x(), 0.5 * dq2.y(), 0.5 * dq2.z()))
										.normalized();
		}
	}

private:
	int m_body1, m_body2;
	Vector3d m_anchor, m_axis;
	double m1 = 1.0, m2 = 1.0; // Inverse masses
};

int main()
{
	// Simulation parameters
	double dt = 1.0 / 60.0;
	double gravity = -9.81;
	int numSubsteps = 10;
	int numIterations = 5;

	// Initial positions and orientations
	vector<Vector3d> positions = {Vector3d(0, 0, 0), Vector3d(0, 0, 3)};
	vector<Quaterniond> orientations = {Quaterniond::Identity(), Quaterniond::Identity()};

	// Hinge constraint
	Vector3d hingeAnchor(0, 0, 1.5);
	Vector3d hingeAxis(1, 0, 0);
	HingeConstraint hingeConstraint(0, 1, hingeAnchor, hingeAxis);

	// Main simulation loop
	for(int i = 0; i < 10; ++i)
	{
		// Unconstrained step (explicit Euler integration)
		for(int j = 0; j < positions.size(); ++j)
		{
			Vector3d velocity(0, gravity * dt, 0);
			positions[j] += velocity * dt;
		}

		// Constrained step (XPBD)
		for(int substep = 0; substep < numSubsteps; ++substep)
		{
			// Gauss-Seidel iteration
			for(int iteration = 0; iteration < numIterations; ++iteration)
			{
				hingeConstraint.solveConstraint(positions, orientations, 1e-6, dt);
				cout << "position: \n"
					 << positions[1] << "\nrotation: \n"
					 << orientations[1] << endl;
			}
		}

		// Print or visualize the current state of the bodies
		// ...
	}

	return 0;
}