#ifndef SIM_H
#define SIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "a.h"
#include "m.h"

struct sim_vehicle;
struct sim;

struct sim* sim_new();
int sim_step(struct sim*, float dt);
void sim_add_block(struct sim*, struct vec3* points, int n_points);
struct sim_vehicle* sim_get_vehicle(struct sim* sim, int i);

void sim_vehicle_get_tx(struct sim_vehicle* vehicle, struct mat44* tx);
void sim_vehicle_get_wheel_tx(struct sim_vehicle* vehicle, struct mat44* tx, int i);
void sim_vehicle_ctrl(struct sim_vehicle* vehicle, int accel, int brake, int steer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif/*SIM_H*/
