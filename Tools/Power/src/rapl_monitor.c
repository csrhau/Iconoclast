#include "rapl_monitor.h"
#include "rapl_defs.h"
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

static uint64_t read_msr(int which_, int msr_fh) {
  uint64_t data;
  if (pread(msr_fh, &data, sizeof data, which_) != sizeof(data)) {
    fprintf(stderr, "Screwed up rapl read!!\n");
  }
  return data;
}

void rapl_init(struct PowerState * pstate){
  uint64_t result = read_msr(MSR_RAPL_POWER_UNIT, pstate->msr_fh);
  pstate->power_units = pow(0.5,(double)(result&0xf));
  pstate->energy_units = pow(0.5,(double)((result>>8)&0x1f));
  pstate->time_units = pow(0.5,(double)((result>>16)&0xf));
  pstate->pkg_pwr = 0;
  pstate->pp0_pwr = 0;
  uint64_t initial_value = read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh);
  do {
    pstate->pkg_reg_value = read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh);
    pstate->pp0_reg_value = read_msr(MSR_PP0_ENERGY_STATUS, pstate->msr_fh);
  } while (pstate->pkg_reg_value == initial_value); // ensure a fresh reading
}

void rapl_refresh(struct PowerState * pstate){
  uint64_t pkg_reg_init = pstate->pkg_reg_value;
  uint64_t pp0_reg_init = pstate->pp0_reg_value;
  uint64_t pkg_reg_curr = read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh);
  uint64_t pp0_reg_curr = read_msr(MSR_PP0_ENERGY_STATUS, pstate->msr_fh);
  if ( pkg_reg_init > pkg_reg_curr || pp0_reg_init > pp0_reg_curr) {
    fprintf(stderr, "Wrap around detected!\n"
                     "\tpkg_init:\t%ld\n"
                     "\tpkg_curr:\t%ld\n"
                     "\tpp0_init:\t%ld\n"
                     "\tpp0_curr:\t%ld\n", 
                     pkg_reg_init, pkg_reg_curr, pp0_reg_init, pp0_reg_curr);
  }
  pstate->pkg_reg_value = pkg_reg_curr;
  pstate->pp0_reg_value = pp0_reg_curr;
  pstate->pkg_pwr += (pkg_reg_curr - pkg_reg_init) * pstate->energy_units;
  pstate->pp0_pwr += (pp0_reg_curr - pp0_reg_init) * pstate->energy_units;
}


void rapl_finalize(struct PowerState * pstate){
  // TODO disable signal
  uint64_t case_reads = 3;
  uint64_t test_reads = 3;
  const uint64_t pkg_init = pstate->pkg_reg_value;
  const uint64_t pp0_init = pstate->pp0_reg_value;
  const uint64_t pkg_pre = read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh);
  const uint64_t pp0_pre = read_msr(MSR_PP0_ENERGY_STATUS, pstate->msr_fh);
  while(read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh) == pkg_pre) {
    ++case_reads;
  }
  const uint64_t pkg_post = read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh);
  const uint64_t pp0_post = read_msr(MSR_PP0_ENERGY_STATUS, pstate->msr_fh);
  while(read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh) == pkg_post) {
    ++test_reads;
  }
  const uint64_t pkg_exit = read_msr(MSR_PKG_ENERGY_STATUS, pstate->msr_fh);
  const uint64_t pp0_exit = read_msr(MSR_PP0_ENERGY_STATUS, pstate->msr_fh);

 
  double ratio =  ((double) case_reads)   / ((double) test_reads);

  // Should be max of what it is, post - (exit - post) * case_reads / test_reads)
  pstate->pkg_reg_value = fmax(pkg_pre, (uint64_t) pkg_post - (pkg_exit - pkg_post) 
                                                           * ratio);
  pstate->pp0_reg_value = fmax(pp0_pre, (uint64_t) pp0_post - (pp0_exit - pp0_post) 
                                                           * ratio);
  pstate->pkg_pwr += (pstate->pkg_reg_value - pkg_init) * pstate->energy_units;
  pstate->pp0_pwr += (pstate->pp0_reg_value - pp0_init) * pstate->energy_units;
}
