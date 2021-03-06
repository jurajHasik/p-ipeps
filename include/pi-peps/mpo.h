#ifndef __MPO_
#define __MPO_

#include "pi-peps/config.h"
#include "pi-peps/ctm-cluster.h"
DISABLE_WARNINGS
#include "itensor/all.h"
ENABLE_WARNINGS

// ----- Main MPO Structures ------------------------------------------
// Index names of 3-site MPO indices
const std::string TAG_MPO3S_PHYS1 = "I_MPO3S_S1";
const std::string TAG_MPO3S_PHYS2 = "I_MPO3S_S2";
const std::string TAG_MPO3S_PHYS3 = "I_MPO3S_S3";
const std::string TAG_MPO3S_12LINK = "I_MPO3S_L12";
const std::string TAG_MPO3S_23LINK = "I_MPO3S_L23";

const char* const TAG_IT_MPOLINK = "MPOlink";

// types for auxiliary indices of MPO tensors
const auto MPOLINK = itensor::IndexType(TAG_IT_MPOLINK);

// operator acting on n-sites
struct OpNS {
  std::string uuid;

  // number of sites over which this operator acts
  int nSite;

  itensor::ITensor op;

  // siteId of sites on which the MPOs are constructed (if any)
  std::vector<std::string> siteIds;

  // physical indices of MPO's
  std::vector<itensor::Index> pi;

  OpNS();

  OpNS(int n);

  OpNS(int n, std::string uuid);
};

struct MpoNS : OpNS {
  // individual MPOs
  std::vector<itensor::ITensor> mpo;

  // aux indices of MPO's
  std::vector<itensor::Index> ai;

  MpoNS();

  MpoNS(int n);

  MpoNS(int n, std::string uuid);

  // MpoNS(MpoNS const& op);
};

/*
 * this struct holds instance of particular 3-site MPO composed
 * by three tensors
 *
 *    s1'              s2'              s3'
 *    |                |                |
 *   |H1|--I_MPO3s12--|H2|--I_MPO3s23--|H3|
 *    |                |                |
 *    s1               s2               s3
 *
 * exposing the physical indices s1,s2,s3
 *
 */
struct MPO_3site : MpoNS {
  itensor::ITensor &H1, &H2, &H3;

  // expose physical indices
  itensor::Index &Is1, &Is2, &Is3;

  // expose internal indices
  itensor::Index &a12, &a23;

  // constructor
  MPO_3site();

  // copy-constructor
  MPO_3site(MPO_3site const& op);
};

struct MPO_2site : MpoNS {
  itensor::ITensor &H1, &H2;

  // expose physical indices
  itensor::Index &Is1, &Is2;

  // expose auxiliary indices
  itensor::Index& a12;

  // constructor
  MPO_2site();

  // copy-constructor
  MPO_2site(MPO_2site const& op);
};

// ----- END Main MPO Structures --------------------------------------

// ----- MPOs construction --------------------------------------------
MPO_2site symmMPO2Sdecomp(itensor::ITensor const& u12,
                          itensor::Index const& s1,
                          itensor::Index const& s2,
                          bool dbg = false);

MPO_3site symmMPO3Sdecomp(itensor::ITensor const& u123,
                          itensor::Index const& s1,
                          itensor::Index const& s2,
                          itensor::Index const& s3,
                          bool dbg = false);

MPO_3site ltorMPO3Sdecomp(itensor::ITensor const& u123,
                          itensor::Index const& s1,
                          itensor::Index const& s2,
                          itensor::Index const& s3,
                          bool dbg = false);

MPO_3site ltorMPO2StoMPO3Sdecomp(itensor::ITensor const& u123,
                                 itensor::Index const& s1,
                                 itensor::Index const& s2,
                                 bool dbg = false);
// ----- END MPOs construction ----------------------------------------

std::ostream& operator<<(std::ostream& s, MPO_2site const& mpo2s);

std::ostream& operator<<(std::ostream& s, MPO_3site const& mpo3s);

#endif
