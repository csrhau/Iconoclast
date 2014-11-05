#ifndef RAPL_MONITOR_H
#define RAPL_MONITOR_H

#include <stdint.h>

struct PowerState {
  int msr_fh;
  uint64_t pkg_reg_value;
  uint64_t pp0_reg_value;
  double power_units;
  double energy_units;
  double time_units;
  double pkg_pwr;
  double pp0_pwr;
};

void rapl_init(struct PowerState * pstate);
void rapl_refresh(struct PowerState * pstate);
void rapl_finalize(struct PowerState * pstate);

#endif
