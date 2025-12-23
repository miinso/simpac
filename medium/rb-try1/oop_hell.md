```cpp
bool addRigidBodyContactConstraint(const unsigned int rbIndex1, const unsigned int rbIndex2,
					const Vector3r &cp1, const Vector3r &cp2,
					const Vector3r &normal, const Real dist,
					const Real restitutionCoeff, const Real frictionCoeff);

bool SimulationModel::addRigidBodyContactConstraint(const unsigned int rbIndex1, const unsigned int rbIndex2,
	const Vector3r &cp1, const Vector3r &cp2,
	const Vector3r &normal, const Real dist,
	const Real restitutionCoeff, const Real frictionCoeff)
{
	m_rigidBodyContactConstraints.emplace_back(RigidBodyContactConstraint());
	RigidBodyContactConstraint &cc = m_rigidBodyContactConstraints.back();
	const bool res = cc.initConstraint(*this, rbIndex1, rbIndex2, cp1, cp2, normal, dist, restitutionCoeff, m_contactStiffnessRigidBody, frictionCoeff);
	if (!res)
		m_rigidBodyContactConstraints.pop_back();
	return res;
}

//

class RigidBodyContactConstraint
	{
	public:
		static int TYPE_ID;
		/** indices of the linked bodies */
		std::array<unsigned int, 2> m_bodies;
		Real m_stiffness;
		Real m_frictionCoeff;
		Real m_sum_impulses;
		Eigen::Matrix<Real, 3, 5, Eigen::DontAlign> m_constraintInfo;

		RigidBodyContactConstraint() {}
		~RigidBodyContactConstraint() {}
		virtual int &getTypeId() const { return TYPE_ID; }

		bool initConstraint(SimulationModel &model, const unsigned int rbIndex1, const unsigned int rbIndex2,
			const Vector3r &cp1, const Vector3r &cp2,
			const Vector3r &normal, const Real dist,
			const Real restitutionCoeff, const Real stiffness, const Real frictionCoeff);
		virtual bool solveVelocityConstraint(SimulationModel &model, const unsigned int iter);
	};

//

//////////////////////////////////////////////////////////////////////////
// RigidBodyContactConstraint
//////////////////////////////////////////////////////////////////////////
bool RigidBodyContactConstraint::initConstraint(SimulationModel &model, const unsigned int rbIndex1, const unsigned int rbIndex2,
		const Vector3r &cp1, const Vector3r &cp2,
		const Vector3r &normal, const Real dist,
		const Real restitutionCoeff, const Real stiffness, const Real frictionCoeff)
{
	m_stiffness = stiffness;
	m_frictionCoeff = frictionCoeff;

	m_bodies[0] = rbIndex1;
	m_bodies[1] = rbIndex2;
	SimulationModel::RigidBodyVector &rb = model.getRigidBodies();
	RigidBody &rb1 = *rb[m_bodies[0]];
	RigidBody &rb2 = *rb[m_bodies[1]];

	m_sum_impulses = 0.0;

	return PositionBasedRigidBodyDynamics::init_RigidBodyContactConstraint(
		rb1.getInvMass(),
		rb1.getPosition(),
		rb1.getVelocity(),
		rb1.getInertiaTensorInverseW(),
		rb1.getRotation(),
		rb1.getAngularVelocity(),
		rb2.getInvMass(),
		rb2.getPosition(),
		rb2.getVelocity(),
		rb2.getInertiaTensorInverseW(),
		rb2.getRotation(),
		rb2.getAngularVelocity(),
		cp1, cp2, normal, restitutionCoeff,
		m_constraintInfo);
}

bool RigidBodyContactConstraint::solveVelocityConstraint(SimulationModel &model, const unsigned int iter)
{
	SimulationModel::RigidBodyVector &rb = model.getRigidBodies();

	RigidBody &rb1 = *rb[m_bodies[0]];
	RigidBody &rb2 = *rb[m_bodies[1]];

	Vector3r corr_v1, corr_v2;
	Vector3r corr_omega1, corr_omega2;
	const bool res = PositionBasedRigidBodyDynamics::velocitySolve_RigidBodyContactConstraint(
		rb1.getInvMass(),
		rb1.getPosition(),
		rb1.getVelocity(),
		rb1.getInertiaTensorInverseW(),
		rb1.getAngularVelocity(),
		rb2.getInvMass(),
		rb2.getPosition(),
		rb2.getVelocity(),
		rb2.getInertiaTensorInverseW(),
		rb2.getAngularVelocity(),
		m_stiffness,
		m_frictionCoeff,
		m_sum_impulses,
		m_constraintInfo,
		corr_v1,
		corr_omega1,
		corr_v2,
		corr_omega2);

	if (res)
	{
		if (rb1.getMass() != 0.0)
		{
			rb1.getVelocity() += corr_v1;
			rb1.getAngularVelocity() += corr_omega1;
		}
		if (rb2.getMass() != 0.0)
		{
			rb2.getVelocity() += corr_v2;
			rb2.getAngularVelocity() += corr_omega2;
		}
	}
	return res;
}

//

// ----------------------------------------------------------------------------------------------
bool PositionBasedRigidBodyDynamics::init_RigidBodyContactConstraint(
	const Real invMass0,							// inverse mass is zero if body is static
	const Vector3r &x0,						// center of mass of body 0
	const Vector3r &v0,						// velocity of body 0
	const Matrix3r &inertiaInverseW0,		// inverse inertia tensor (world space) of body 0
	const Quaternionr &q0,					// rotation of body 0
	const Vector3r &omega0,					// angular velocity of body 0
	const Real invMass1,							// inverse mass is zero if body is static
	const Vector3r &x1,						// center of mass of body 1
	const Vector3r &v1,						// velocity of body 1
	const Matrix3r &inertiaInverseW1,		// inverse inertia tensor (world space) of body 1
	const Quaternionr &q1,					// rotation of body 1
	const Vector3r &omega1,					// angular velocity of body 1
	const Vector3r &cp0,						// contact point of body 0
	const Vector3r &cp1,						// contact point of body 1
	const Vector3r &normal,					// contact normal in body 1
	const Real restitutionCoeff,					// coefficient of restitution
	Eigen::Matrix<Real, 3, 5, Eigen::DontAlign> &constraintInfo)
{
	// constraintInfo contains
	// 0:	contact point in body 0 (global)
	// 1:	contact point in body 1 (global)
	// 2:	contact normal in body 1 (global)
	// 3:	contact tangent (global)
	// 0,4:  1.0 / normal^T * K * normal
	// 1,4: maximal impulse in tangent direction
	// 2,4: goal velocity in normal direction after collision

	// compute goal velocity in normal direction after collision
	const Vector3r r0 = cp0 - x0;
	const Vector3r r1 = cp1 - x1;

	const Vector3r u0 = v0 + omega0.cross(r0);
	const Vector3r u1 = v1 + omega1.cross(r1);
	const Vector3r u_rel = u0 - u1;
	const Real u_rel_n = normal.dot(u_rel);

	constraintInfo.col(0) = cp0;
	constraintInfo.col(1) = cp1;
	constraintInfo.col(2) = normal;

	// tangent direction
	Vector3r t = u_rel - u_rel_n*normal;
	Real tl2 = t.squaredNorm();
	if (tl2 > 1.0e-6)
		t *= static_cast<Real>(1.0) / sqrt(tl2);

	constraintInfo.col(3) = t;

	// determine K matrix
	Matrix3r K1, K2;
	computeMatrixK(cp0, invMass0, x0, inertiaInverseW0, K1);
	computeMatrixK(cp1, invMass1, x1, inertiaInverseW1, K2);
	Matrix3r K = K1 + K2;

	constraintInfo(0, 4) = static_cast<Real>(1.0) / (normal.dot(K*normal));

	// maximal impulse in tangent direction
	constraintInfo(1, 4) = static_cast<Real>(1.0) / (t.dot(K*t)) * u_rel.dot(t);

	// goal velocity in normal direction after collision
	constraintInfo(2, 4) = 0.0;
	if (u_rel_n < 0.0)
		constraintInfo(2, 4) = -restitutionCoeff * u_rel_n;

	return true;
}

///

//--------------------------------------------------------------------------------------------
bool PositionBasedRigidBodyDynamics::velocitySolve_RigidBodyContactConstraint(
	const Real invMass0,							// inverse mass is zero if body is static
	const Vector3r &x0, 						// center of mass of body 0
	const Vector3r &v0,						// velocity of body 0
	const Matrix3r &inertiaInverseW0,		// inverse inertia tensor (world space) of body 0
	const Vector3r &omega0,					// angular velocity of body 0
	const Real invMass1,							// inverse mass is zero if body is static
	const Vector3r &x1, 						// center of mass of body 1
	const Vector3r &v1,						// velocity of body 1
	const Matrix3r &inertiaInverseW1,		// inverse inertia tensor (world space) of body 1
	const Vector3r &omega1,					// angular velocity of body 1
	const Real stiffness,							// stiffness parameter of penalty impulse
	const Real frictionCoeff,						// friction coefficient
	Real &sum_impulses,							// sum of all impulses
	Eigen::Matrix<Real, 3, 5, Eigen::DontAlign> &constraintInfo,		// precomputed contact info
	Vector3r &corr_v0, Vector3r &corr_omega0,
	Vector3r &corr_v1, Vector3r &corr_omega1)
{
	// constraintInfo contains
	// 0:	contact point in body 0 (global)
	// 1:	contact point in body 1 (global)
	// 2:	contact normal in body 1 (global)
	// 3:	contact tangent (global)
	// 0,4:  1.0 / normal^T * K * normal
	// 1,4: maximal impulse in tangent direction
	// 2,4: goal velocity in normal direction after collision

	if ((invMass0 == 0.0) && (invMass1 == 0.0))
		return false;

	const Vector3r &connector0 = constraintInfo.col(0);
	const Vector3r &connector1 = constraintInfo.col(1);
	const Vector3r &normal = constraintInfo.col(2);
	const Vector3r &tangent = constraintInfo.col(3);

	// 1.0 / normal^T * K * normal
	const Real nKn_inv = constraintInfo(0, 4);

	// penetration depth
	const Real d = normal.dot(connector0 - connector1);

	// maximal impulse in tangent direction
	const Real pMax = constraintInfo(1, 4);

	// goal velocity in normal direction after collision
	const Real goal_u_rel_n = constraintInfo(2, 4);

	const Vector3r r0 = connector0 - x0;
	const Vector3r r1 = connector1 - x1;

	const Vector3r u0 = v0 + omega0.cross(r0);
	const Vector3r u1 = v1 + omega1.cross(r1);

	const Vector3r u_rel = u0-u1;
	const Real u_rel_n = u_rel.dot(normal);
	const Real delta_u_reln = goal_u_rel_n - u_rel_n;

	Real correctionMagnitude = nKn_inv * delta_u_reln;

	if (correctionMagnitude < -sum_impulses)
		correctionMagnitude = -sum_impulses;

	// add penalty impulse to counteract penetration
	if (d < 0.0)
		correctionMagnitude -= stiffness * nKn_inv * d;


	Vector3r p(correctionMagnitude * normal);
	sum_impulses += correctionMagnitude;

	// dynamic friction
	const Real pn = p.dot(normal);
	if (frictionCoeff * pn > pMax)
		p -= pMax * tangent;
	else if (frictionCoeff * pn < -pMax)
		p += pMax * tangent;
	else
		p -= frictionCoeff * pn * tangent;

	if (invMass0 != 0.0)
	{
		corr_v0 = invMass0*p;
		corr_omega0 = inertiaInverseW0 * (r0.cross(p));
	}

	if (invMass1 != 0.0)
	{
		corr_v1 = -invMass1*p;
		corr_omega1 = inertiaInverseW1 * (r1.cross(-p));
	}

	return true;
}
```
