#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#include "btBulletDynamicsCommon.h"
#pragma clang diagnostic pop

#include "sim.h"

#define WORLD_MAX (1000)
#define WHEEL_RADIUS (0.3)

struct sim_vehicle {
	btRaycastVehicle::btVehicleTuning tuning;
	btVehicleRaycaster* vehicleRayraster;
	btRaycastVehicle* raycastVehicle;
	btRigidBody* chassis;
	btVector3 chassis_extents;

	btRigidBody* make_chassis(btDynamicsWorld* world)
	{
		chassis_extents = btVector3(0.3, 0.1, 1);
		btCollisionShape* shape = new btBoxShape(chassis_extents);

		float mass = 600;
		btVector3 local_inertia(0,0,0);
		shape->calculateLocalInertia(mass, local_inertia);

		btTransform tx;
		tx.setIdentity();
		tx.setOrigin(btVector3(10,20,10)); // XXX see track_init_demo()
		tx.setRotation(btQuaternion(-1.5,0,0));
		btDefaultMotionState* mstate = new btDefaultMotionState(tx);
		btRigidBody::btRigidBodyConstructionInfo cinfo(mass, mstate, shape, local_inertia);
		btRigidBody* body = new btRigidBody(cinfo);
		body->setContactProcessingThreshold(1e6); // ???
		world->addRigidBody(body);
		body->setActivationState(DISABLE_DEACTIVATION);
		return body;
	}

	void initialize(btDynamicsWorld* world)
	{
		chassis = make_chassis(world);
		vehicleRayraster = new btDefaultVehicleRaycaster(world);
		raycastVehicle = new btRaycastVehicle(tuning, chassis, vehicleRayraster);
		world->addVehicle(raycastVehicle);
		raycastVehicle->setCoordinateSystem(0,1,2);
		btVector3 dir(0,-1,0);
		btVector3 axle(-1,0,0);
		float suspension_rest_length = 0.6;
		for (int i = 0; i < 4; i++) {
			bool is_front_wheel = i < 2;
			float dside = 0.5;
			float dx = i&1 ? dside : -dside;
			float dfront = 1.6;
			//float h = -0.1;
			float h = 0.2;
			float dz = is_front_wheel ? dfront : -dfront;
			btVector3 point(dx, h, dz);
			btWheelInfo& wheel = raycastVehicle->addWheel(point, dir, axle, suspension_rest_length, WHEEL_RADIUS, tuning, is_front_wheel);
			wheel.m_suspensionStiffness = 10;
			wheel.m_wheelsDampingRelaxation = 2.3;
			wheel.m_wheelsDampingCompression = 4.4;
			wheel.m_frictionSlip = 100;
			wheel.m_rollInfluence = 0.1;
		}
		//raycastVehicle->getRigidBody()->applyImpulse(btVector3(-22,-2,-2), btVector3(10,0,0));
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

		btVector3 worldMin(-WORLD_MAX, -WORLD_MAX, -WORLD_MAX);
		btVector3 worldMax(WORLD_MAX, WORLD_MAX, WORLD_MAX);
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

		add_ground();
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
		body->setContactProcessingThreshold(1e3); // ???
		world->addRigidBody(body);
	}

	void add_ground()
	{
		btVector3 extents(WORLD_MAX, 1, WORLD_MAX);
		btBoxShape* shape = new btBoxShape(extents);

		btTransform tx;
		tx.setIdentity();
		btVector3 translation(0,-1,0);
		tx.setOrigin(translation);
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

static void mat44_from_btTransform(struct mat44* tx, btTransform btx)
{
	mat44_set_identity(tx);
	btx.getOpenGLMatrix(&tx->s[0]);
}

static void vec3_from_btVector3(struct vec3* v, btVector3 btv)
{
	for (int i = 0; i < 3; i++) v->s[i] = btv[i];
}

void sim_vehicle_get_tx(struct sim_vehicle* vehicle, struct mat44* tx)
{
	btRigidBody* body = vehicle->raycastVehicle->getRigidBody();
	//btRigidBody* body = vehicle->chassis;

	btTransform transform;
	body->getMotionState()->getWorldTransform(transform);

	struct mat44 a;
	mat44_from_btTransform(&a, transform);
	mat44_inverse(tx, &a);
}

void sim_vehicle_render(struct render* render, struct sim_vehicle* vehicle)
{
	// render wheels
	for (int w = 0; w < 4; w++) {
		btWheelInfo& wheel = vehicle->raycastVehicle->getWheelInfo(w);
		struct mat44 wtx;
		mat44_from_btTransform(&wtx, wheel.m_worldTransform);
		render_a_wheel(render, &wtx, WHEEL_RADIUS, 0.08);
	}

	// render chassis
	{
		struct mat44 wtx;
		btTransform btx;
		vehicle->chassis->getMotionState()->getWorldTransform(btx);
		mat44_from_btTransform(&wtx, btx);
		struct vec3 extents;
		vec3_from_btVector3(&extents, vehicle->chassis_extents);
		render_box(render, &wtx, &extents);
	}
}

void sim_vehicle_ctrl(struct sim_vehicle* vehicle, int accel, int brake, int steer)
{
	for (int w = 0; w < 4; w++) {
		/*
		float aforce = accel ? 1000 : 0;
		float bforce = brake ? 300 : 0;
		vehicle->raycastVehicle->applyEngineForce(aforce, w);
		vehicle->raycastVehicle->setBrake(bforce, w);
		*/
	}
	float aforce = accel ? 1000 : 0;
	float bforce = brake ? 100 : 0;
	for (int w = 0; w < 2; w++) {
		vehicle->raycastVehicle->applyEngineForce(aforce, w);
		vehicle->raycastVehicle->setSteeringValue((float)steer * -0.4f, w);

		vehicle->raycastVehicle->setBrake(bforce, w+2);

	}
	//printf("%f\n", vehicle->raycastVehicle->getCurrentSpeedKmHour());
}

void sim_vehicle_visualize(struct sim_vehicle* vehicle, struct render* render)
{
	//render_begin_color(render);
	for (int w = 0; w < 4; w++) {
		btWheelInfo& wheel = vehicle->raycastVehicle->getWheelInfo(w);
		struct vec3 o, v;
		vec3_from_btVector3(&o, wheel.m_raycastInfo.m_hardPointWS);
		vec3_from_btVector3(&v, wheel.m_raycastInfo.m_wheelDirectionWS);
		render_draw_vector(render, &o, &v, NULL);
	}
	//render_end_color(render);
}

} /* extern "C" */

