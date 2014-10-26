#ifndef SIM_H
#define SIM_H

#ifdef __cplusplus
extern "C" {
#endif

struct sim;

struct sim* sim_new();
int sim_step(struct sim*, float dt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif/*SIM_H*/
