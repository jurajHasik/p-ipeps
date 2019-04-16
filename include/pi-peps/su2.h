#ifndef __SU2_
#define __SU2_

#include "pi-peps/config.h"
#include <algorithm>
#include <cmath>
#include <complex>
DISABLE_WARNINGS
#include "itensor/all.h"
ENABLE_WARNINGS

// Supported types of 1-site operators
enum SU2O {
  SU2_Id,    // Identity
  SU2_S_Z,   // Projection on S_z
  SU2_S_Z2,  // S_z^2
  SU2_S_P,   // S_plus
  SU2_S_M    // S_minus
};

/*
 * Return spin operator with indices s, prime(s)
 *
 */
itensor::ITensor SU2_getSpinOp(SU2O su2o,
                               itensor::Index const& s,
                               bool DBG = false);

itensor::ITensor SU2_getRotOp(itensor::Index const& s);

itensor::ITensor SU2_applyRot(itensor::Index const& s, itensor::ITensor const& op);

double SU2_getCG(int j1, int j2, int j, int m1, int m2, int m);

int Factorial(int x);

#endif
