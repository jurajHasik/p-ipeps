#ifndef __CLS_EV_BUILDER_
#define __CLS_EV_BUILDER_

#include "pi-peps/config.h"
#include <iostream>
#include <string>
DISABLE_WARNINGS
#include "itensor/all.h"
ENABLE_WARNINGS
#include "pi-peps/ctm-cluster-global.h"
#include "pi-peps/ctm-cluster-io.h"
#include "pi-peps/ctm-cluster.h"
#include "pi-peps/ctm-env.h"
#include "pi-peps/mpo.h"
#include "pi-peps/su2.h"

class EVBuilder {
 public:
  std::string name;

  const Cluster* p_cluster;
  const CtmEnv* p_ctmEnv;

  Cluster cls;
  CtmData_Full cd_f;

  // Basic Constructor
  EVBuilder(std::string in_name, Cluster const& cls, CtmEnv const& env);

  // Supported types of 1-site operators
  enum MPO_1S {
    MPO_Id,    // Identity
    MPO_S_Z,   // Projection on S_z
    MPO_S_Z2,  // S_z^2
    MPO_S_P,   // S_plus
    MPO_S_M    // S_minus
  };

  // Get on-site contracted tensor <T(bra)|MPO|T(ket)>,
  // with prime level "l"
  MpoNS getTOT(MPO_1S mpo,
               Vertex const& v,
               int primeLvl,
               bool DBG = false) const;

  MpoNS getTOT(MPO_1S mpo,
               std::string siteId,
               int primeLvl,
               bool DBG = false) const;

  // Assume op to be tensor of at least rank 2 with exactly two copies
  // of single PHYS index with primeLevel 0 and 1
  MpoNS getTOT(itensor::ITensor const& op,
               std::string siteId,
               int primeLvl,
               bool DBG = false) const;

  // Compute expectation value of 1-site operator O
  /*
   *  Arg op - result of getTOT = <bra|op|ket> with indices in accordance
   *           to definition of TN
   *  Arg site - defines the position within cluster, where original on-site
   *             tensor is replaced with tensor op
   *
   */

  // compute ev_1sO using environment of a single site
  double eV_1sO_1sENV(MPO_1S op1s, Vertex const& v, bool DBG = false) const;

  double eV_1sO_1sENV(MpoNS const& op, Vertex const& v, bool DBG = false) const;

  std::vector<double> eeCorner_1s_inner(Vertex const& v,
                                        bool DBG = false) const;

  std::vector<double> eeCorner_1s_outer(Vertex const& v,
                                        bool DBG = false) const;

  // Supported types of 2-site operators
  enum OP_2S {
    OP2S_Id,         // Identity
    OP2S_AKLT_S2_H,  // Hamiltonian - P(rojector)^{S=4} on S=4 subspace
    OP2S_SS,         // Hamiltonian - NN-Heisenberg
    OP2S_SZSZ
  };

  enum OP_4S { OP4S_ID, OP4S_Q };

  // Compute expectation value of 2-site operator O given
  // by its decomposition into MPOs OA * OB = O placed on siteA and siteB
  // within the cluster surrounded by environment

  // compute ev_2sO using minimal rectangle defined by s1 and s2
  double eV_2sO_Rectangle(
    std::pair<itensor::ITensor, itensor::ITensor> const& Op,
    Vertex const& v1,
    Vertex const& v2,
    bool DBG = false) const;

  // compute norm of TN defined by sites s1, s2 (with ID operators)
  double getNorm_Rectangle(bool DBG, Vertex const& v1, Vertex const& v2) const;

  // contract network using minimal rectangle defined by s1 and s2
  // with Op.first inserted on s1 and Op.second inserted in s2
  double get2SOPTN(bool DBG,
                   std::pair<itensor::ITensor, itensor::ITensor> const& Op,
                   Vertex const& v1,
                   Vertex const& v2) const;

  itensor::ITensor redDenMat_2S(Vertex const& v1,
                                Vertex const& v2,
                                bool DBG) const;

  itensor::ITensor insert2S(
    bool DBG,
    std::pair<itensor::ITensor, itensor::ITensor> const& Op,
    Vertex const& v1,
    Vertex const& v2) const;

  // coefs[0] * SxSx + coefs[1] * SySy + coefs[2] * SzSz
  double evalSS(Vertex const& v1,
                Vertex const& v2,
                std::vector<double> coefs = {1.0, 1.0, 1.0},
                bool DBG = false) const;

  double eval2Smpo(OP_2S op2s,
                   Vertex const& v1,
                   Vertex const& v2,
                   bool DBG = false) const;

  double eval2Smpo(std::pair<itensor::ITensor, itensor::ITensor> const& Op,
                   Vertex const& v1,
                   Vertex const& v2,
                   bool DBG = false) const;

  double contract2Smpo(OP_2S op2s,
                       Vertex const& v1,
                       Vertex const& v2,
                       bool DBG = false) const;

  double contract2Smpo(std::pair<itensor::ITensor, itensor::ITensor> const& Op,
                       Vertex const& v1,
                       Vertex const& v2,
                       bool DBG = false) const;

  // double eval2Smpo_redDenMat2x1(OP_2S op2s, std::pair<int,int> s1,
  // std::pair<int,int> s2,
  //     bool DBG = false) const;

  // itensor::ITensor redDenMat2x1(std::pair<int,int> s1, std::pair<int,int> s2,
  //     bool DBG = false) const;

  // Correlation function
  // Compute expectation value of two 1-site operators O1, O2
  // where O1 is applied on "site" and O2 "dist" sites to the right
  // in horizontal direction
  /*  _    _    _         _    _    _
   * |C|--|T|--|T|--...--|T|--|T|--|C|
   *  |    |    |         |    |    |
   * |T|--|O1|-|X|--...--|X|-|O2|--|T|
   *  |    |    |         |    |    |
   * |C|--|T|--|T|--...--|T|--|T|--|C|
   *
   *           <--"dist"-->
   * Hence "dist" = 0, means adjacent sites
   *
   */
  // std::vector< std::complex<double> > expVal_1sO1sO_H(MPO_1S o1,
  //     MPO_1S o2, std::pair< int, int > site, int dist, bool dbg = false);

  struct TransferOpVecProd_itensor {
    CtmEnv::DIRECTION dir;
    Vertex v_ref;
    EVBuilder const& ev;

    TransferOpVecProd_itensor(EVBuilder const& ev,
                              Vertex const& v,
                              CtmEnv::DIRECTION dir);

    void operator()(itensor::ITensor& bT, bool DBG = false);
  };

  double analyzeBoundaryVariance(
    Vertex const& v,
    CtmEnv::DIRECTION dir = CtmEnv::DIRECTION::RIGHT,
    bool dbg = false);

  /*
   * Evaluate 2 site operator along diagonal using corner construction
   * Diagonal is defined with respect to a position of O1 given by site s1,
   * with operator O2 being inserted at s1 + [1,1]
   *
   * |C|--|T|---|T|--|C|
   *  |    |     |    |
   * |T|--|O1|--|X|--|T|
   *  |    |     |    |
   * |C|--|X|--|O2|--|T|
   *  |    |    |     |
   * |C|--|T|--|T |--|C|
   *
   */
  double eval2x2Diag11(OP_2S op2s, Vertex const& v1, bool DBG = false) const;

  double eval2x2Diag11(std::pair<itensor::ITensor, itensor::ITensor> const& Op, Vertex const& v1, bool DBG = false) const;

  double contract2x2Diag11(OP_2S op2s,
                                    Vertex const& v1,
                                    bool DBG) const;

  double contract2x2Diag11(std::pair<itensor::ITensor, itensor::ITensor> const& op,
                           Vertex const& v1,
                           bool DBG = false) const;

  // double eval2x2Diag1N1(OP_2S op2s, Vertex const& v1,
  //     bool DBG = false) const;

  // double contract2x2Diag1N1(OP_2S op2s, Vertex const& v1,
  //     bool DBG = false) const;

  /*
   * Evaluate 2 site operator along diagonal using corner construction
   * Diagonal is defined with respect to a position of O1 given by site s1,
   * with operator O2 being inserted at s1 + [-1,+1]
   *
   * |C|--|T|---|T|--|C|
   *  |    |     |    |
   * |T|--|X|--|O1|--|T|
   *  |    |     |    |
   * |C|--|O2|--|X|--|T|
   *  |    |     |    |
   * |C|--|T|---|T|--|C|
   *
   */
  double eval2x2DiagN11(OP_2S op2s, Vertex const& v1, bool DBG = false) const;

  double eval2x2DiagN11(std::pair<itensor::ITensor, itensor::ITensor> const& Op, Vertex const& v1, bool DBG = false) const;

  double contract2x2DiagN11(OP_2S op2s,
                            Vertex const& v1,
                            bool DBG = false) const;

  double contract2x2DiagN11(std::pair<itensor::ITensor, itensor::ITensor> const& op,
                           Vertex const& v1,
                           bool DBG = false) const;

  double eval2x2op4s(OP_4S op4s, Vertex const& v1, bool DBG = false) const;

  // itensor::ITensor getT(itensor::ITensor const& s,
  //     std::array< itensor::Index, 4> const& plToEnv, bool dbg) const;

  // double contract3Smpo2x2(MPO_3site const& mpo3s,
  //     std::vector< std::pair<int,int> > siteSeq, bool dbg = false) const;

  // double contract3Smpo2x2(MPO_3site const& mpo3s,
  //     std::vector<std::string> tn, std::vector<int> pl, bool dbg = false)
  //     const;

  // Correlation function
  // Compute expectation value of two 1-site operators O1, O2
  // spaced by "dist" sites in vertical direction
  /*  _    _    _
   * |C|--|T|--|C|
   *  |    |    |
   * |T|--|O1|-|T|
   *  |    |    |
   * |T|--|X|--|T|   ^
   *  |    |    |    |
   *      ...      "dist"
   *  |    |    |    |
   * |T|--|X|--|T|   V
   *  |    |    |
   * |T|--|O2|-|T|
   *  |    |    |
   * |C|--|T|--|C|
   *
   * Hence "dist" = 0, means adjacent sites
   *
   */
  //  std::complex<double> expVal_1sO1sO_V(int dist,
  //         itensor::ITensor const& op1, itensor::ITensor const& op2);

  // Correlation function
  // Compute expectation value of two 2-site operators O1, O2
  // spaced by "dist" sites in horizontal direction
  /*  _    _    _         _    _    _
   * |C|--|T|--|T|--...--|T|--|T|--|C|
   *  |    |_   |         |   _|    |
   * |T|--|  |-|X|--...--|X|-|  |--|T|
   *  |   |O1|  |         |  |O2|    |
   * |T|--| _|-|X|--...--|X|-|_ |--|T|
   *  |    |    |         |    |    |
   * |C|--|T|--|T|--...--|T|--|T|--|C|
   *
   *           <--"dist"-->
   * Hence "dist" = 0, means adjacent sites
   *
   */
  // std::complex<double> expVal_2sOV2sOV_H(int dist,
  //         Mpo2S const& op1, Mpo2S const& op2);

  // Correlation function
  // Compute expectation value of two 2-site operators O1, O2
  // spaced by "dist" sites in horizontal direction
  /*  _    _    _    _         _    _    _    _
   * |C|--|T|--|T|--|T|--...--|T|--|T|--|T|--|C|
   *  |    |____|    |         |    |____|    |
   * |T|--|__OP1_|--|X|--...--|X|--|_OP2__|--|T|
   *  |    |    |    |         |    |    |    |
   * |C|--|T|--|T|--|T|--...--|T|--|T|--|T|--|C|
   *
   *           <--"dist"-->
   * Hence "dist" = 0, means adjacent sites
   *
   */
  //  std::complex<double> expVal_2sOH2sOH_H(int dist,
  //          Mpo2S const& op1, Mpo2S const& op2);

  MPO_3site get3Smpo(std::string mpo3s, bool DBG = false) const;

  static std::pair<itensor::ITensor, itensor::ITensor> get2SiteSpinOP(
    OP_2S op2s,
    itensor::Index const& sA,
    itensor::Index const& sB,
    bool dbg = false);

  /*
   * wrapper around SU2_getSpinOp(SU2O su2o, itensor::Index const& s)
   * from pi-peps/su2.h
   *
   */
  static itensor::ITensor getSpinOp(MPO_1S mpo,
                                    itensor::Index const& s,
                                    bool DBG = false);

  std::ostream& print(std::ostream& s) const;
};

std::ostream& operator<<(std::ostream& s, EVBuilder const& ev);

#endif
