#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#include "btBulletDynamicsCommon.h"
#pragma clang diagnostic pop

#include "sim.h"

struct sim_vehicle {
	btRaycastVehicle::btVehicleTuning tuning;
	btVehicleRaycaster* vehicleRayraster;
	btRaycastVehicle* raycastVehicle;
	btRigidBody* chassis;

	btRigidBody* make_chassis(btDynamicsWorld* world)
	{
		btVector3 extents(0.3, 0.1, 1);
		btCollisionShape* shape = new btBoxShape(extents);

		float mass = 800;
		btVector3 local_inertia(0,0,0);
		shape->calculateLocalInertia(mass, local_inertia);

		btTransform tx;
		tx.setIdentity();
		tx.setOrigin(btVector3(10,20,10)); // XXX see track_init_demo()
		btDefaultMotionState* mstate = new btDefaultMotionState(tx);
		btRigidBody::btRigidBodyConstructionInfo cinfo(mass, mstate, shape, local_inertia);
		btRigidBody* body = new btRigidBody(cinfo);
		body->setContactProcessingThreshold(1e6); // ???
		world->addRigidBody(body);
		return body;
	}

	void initialize(btDynamicsWorld* world)
	{
		chassis = make_chassis(world);
		vehicleRayraster = new btDefaultVehicleRaycaster(world);
		raycastVehicle = new btRaycastVehicle(tuning, chassis, vehicleRayraster);
		//raycastVehicle->getRigidBody()->applyImpulse(btVector3(10,1,1), btVector3(10,0,0));
	}
};

struct sim {
	class btDynamicsWorld* world;
	class btConstraintSolver* constraintSolver;
	class btBroadphaseInterface* overlappingPairCache;
	class btCollisionDispatcher* dispatcher;
	class btCollisionConfiguration* collisionConfiguration;

	struct sim_vehicle vehicle;

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

		world->setGravity(btVector3(0,-10,0));
	}

	struct sim_vehicle* get_vehicle(int i)
	{
		ASSERT(i == 0);
		return &vehicle;
	}

	void initialize()
	{
		_initialize_world();
		vehicle.initialize(world);
	}

	int step(float dt)
	{
		int max_steps = 128;
		float fixed_dt = 1.0f / 60.0f;
		return world->stepSimulation(dt, max_steps, fixed_dt);
	}

	void add_block(struct vec3* points, int n_points)
	{
		btConvexHullShape* shape = new btConvexHullShape;
		for (int i = 0; i < n_points; i++) {
			struct vec3* p = &points[i];
			btVector3 btp(p->s[0], p->s[1], p->s[2]);
			shape->addPoint(btp);
		}

		btTransform tx;
		tx.setIdentity();

		btDefaultMotionState* mstate = new btDefaultMotionState(tx);
		btRigidBody::btRigidBodyConstructionInfo cinfo(0, mstate, shape);
		btRigidBody* body = new btRigidBody(cinfo);
		body->setContactProcessingThreshold(1e6); // ???
		world->addRigidBody(body);
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

void sim_add_block(struct sim* sim, struct vec3* points, int n_points)
{
	sim->add_block(points, n_points);
}

struct sim_vehicle* sim_get_vehicle(struct sim* sim, int i)
{
	return sim->get_vehicle(i);
}

static void vec3_from_btVector3(struct vec3* v, btVector3& btv)
{
	v->s[0] = btv.getX();
	v->s[1] = btv.getY();
	v->s[2] = btv.getZ();
	//vec3_dump(v);
}

void sim_vehicle_get_tx(struct sim_vehicle* vehicle, struct mat44* tx)
{
	btRigidBody* body = vehicle->raycastVehicle->getRigidBody();
	//btRigidBody* body = vehicle->chassis;

	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);

	btVector3 pos = -transform.getOrigin() - btVector3(0,2,0);

	mat44_set_identity(tx);
	struct vec3 d;
	vec3_from_btVector3(&d, pos);
	mat44_translate(tx, &d);
}

} /* extern "C" */

