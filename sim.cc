#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#include "btBulletDynamicsCommon.h"
#pragma clang diagnostic pop

#include "sim.h"

struct sim {
	class btDynamicsWorld* world;
	class btConstraintSolver* constraintSolver;
	class btBroadphaseInterface* overlappingPairCache;
	class btCollisionDispatcher* dispatcher;
	class btCollisionConfiguration* collisionConfiguration;

	void _initialize_world()
	{
		collisionConfiguration = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfiguration);

		btVector3 worldMin(-1000, -1000, -1000);
		btVector3 worldMax(1000, 1000, 1000);
		overlappingPairCache = new btAxisSweep3(worldMin, worldMax);

		constraintSolver = new btSequentialImpulseConstraintSolver();

		world = new btDiscreteDynamicsWorld(
			dispatcher,
			overlappingPairCache,
			constraintSolver,
			collisionConfiguration
		);

		world->setGravity(btVector3(0,0,-10));
	}

	void initialize()
	{
		_initialize_world();
	}

	int step(float dt)
	{
		int max_steps = 128;
		float fixed_dt = 1.0f / 60.0f;
		return world->stepSimulation(dt, max_steps, fixed_dt);
	}
};

extern "C" {

struct sim* sim_new()
{
	struct sim* sim = new struct sim;
	sim->initialize();
	return sim;
}

int sim_step(struct sim* sim, float dt)
{
	return sim->step(dt);
}

} /* extern "C" */

