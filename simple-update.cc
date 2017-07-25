#include "simple-update.h"

using namespace itensor;

ID_TYPE toID_TYPE(std::string const& idType) {
    if(idType == "ID_TYPE_1") return ID_TYPE_1;
    if(idType == "ID_TYPE_2") return ID_TYPE_2;
    std::cout << "Unsupported ID_TYPE" << std::endl;
    exit(EXIT_FAILURE);
}

OP2S_TYPE toOP2S_TYPE(std::string const& op2sType) {
	if(op2sType == "ID_OP") return ID_OP;
    if(op2sType == "NNH_OP") return NNH_OP;
    std::cout << "Unsupported OP2S_TYPE" << std::endl;
    exit(EXIT_FAILURE);
}

F_MPO3S toF_MPO3S(std::string const& fMpo3s) {
    if(fMpo3s == "F_MPO3S_1") return F_MPO3S_1;
    if(fMpo3s == "F_MPO3S_2") return F_MPO3S_2;
    if(fMpo3s == "F_MPO3S_3") return F_MPO3S_3;
    if(fMpo3s == "F_MPO3S_4") return F_MPO3S_4;
    if(fMpo3s == "F_MPO3S_5") return F_MPO3S_5;
    if(fMpo3s == "F_MPO3S_6") return F_MPO3S_6;
    std::cout << "Unsupported F_MPO3S" << std::endl;
    exit(EXIT_FAILURE);
}

// 2 SITE OPS #########################################################

MPO_2site getMPO2s_Id(int physDim) {
	MPO_2site mpo2s;
	
	// Define physical indices
	mpo2s.Is1 = Index(TAG_MPO3S_PHYS1,physDim,PHYS);
	mpo2s.Is2 = Index(TAG_MPO3S_PHYS2,physDim,PHYS);

	//create a lambda function
	//which returns the square of its argument
	auto sqrt_T = [](double r) { return sqrt(r); };

    ITensor idpI1(mpo2s.Is1,prime(mpo2s.Is1,1));
    ITensor idpI2(mpo2s.Is2,prime(mpo2s.Is2,1));
    for (int i=1;i<=physDim;i++) {
        idpI1.set(mpo2s.Is1(i),prime(mpo2s.Is1,1)(i),1.0);
        idpI2.set(mpo2s.Is2(i),prime(mpo2s.Is2,1)(i),1.0);
    }

    ITensor id2s = idpI1*idpI2;

    /*
     *  s1'                    s2' 
     *   |                      |
     *  |H1|--a1--<SV_12>--a2--|H2|
     *   |                      |
     *  s1                     s2
     *
     */
    mpo2s.H1 = ITensor(mpo2s.Is1,prime(mpo2s.Is1));
    ITensor SV_12;
    svd(id2s,mpo2s.H1,SV_12,mpo2s.H2);

    PrintData(mpo2s.H1);
    PrintData(SV_12);
    Print(mpo2s.H2);

    Index a1 = commonIndex(mpo2s.H1,SV_12);
    Index a2 = commonIndex(SV_12,mpo2s.H2);

    // Define aux indices linking the on-site MPOs
	Index iMPO3s12(TAG_MPO3S_12LINK,a1.m(),MPOLINK);

	/*
	 * Split obtained SVD values symmetricaly and absorb to obtain
	 * final tensor H1 and intermediate tensor temp
	 *
     *  s1'                                     s2' 
     *   |                                       |
     *  |H1|--a1--<SV_12>^1/2--<SV_12>^1/2--a2--|H2|
     *   |                                       |
     *  s1                                      s2
     *
     */
    SV_12.apply(sqrt_T);
    mpo2s.H1 = ( mpo2s.H1 * SV_12 )*delta(a2,iMPO3s12);
    mpo2s.H2 = ( mpo2s.H2 * SV_12 )*delta(a1,iMPO3s12);
    
    // ----- analyze signs of largest elements in H1, H2 and switch to 
    // positive sign -1 * -1 => +1 * +1 --------------------------------
    double m1, m2;
	double m = 0.;
	double sign = 0.;
    auto max_m = [&m, &sign](double d) {
        if(std::abs(d) > m) {
        	sign = (d > 0) ? 1 : ((d < 0) ? -1 : 0);
         	m = std::abs(d);
        }
    };

    mpo2s.H1.visit(max_m);
    m1 = m*sign;
    m = 0.;
    mpo2s.H2.visit(max_m);
    m2 = m*sign;

    std::cout <<"m1: "<< m1 <<" m2: "<< m2 << std::endl;

	if ( m1*m2 > 0 ) {
		mpo2s.H1 /= m1;
		mpo2s.H2 /= m2;
	}
	// ----- end sign analysis ----------------------------------------

    PrintData(mpo2s.H1);
    PrintData(mpo2s.H2);

	return mpo2s;
}

MPO_2site getMPO2s_NNH(int z, double tau, double J, double h) {
	MPO_2site mpo2s;
	int physDim = 2; // dimension of Hilbert space of spin s=1/2 DoF
	
	// Define physical indices
	mpo2s.Is1 = Index(TAG_MPO3S_PHYS1,physDim,PHYS);
	mpo2s.Is2 = Index(TAG_MPO3S_PHYS2,physDim,PHYS);

	//create a lambda function
	//which returns the square of its argument
	auto sqrt_T = [](double r) { return sqrt(r); };

	// Define the individual tensor elements
	// The values of index enumerate local spin s=1/2 basis as follows
	// Is1 AND Is2 {1 -> s_z=-1/2, 2 -> s_z=1/2}
    // Thus tensor product of indices Is1(x)Is2 gives
    // {1->-1/2 -1/2, 2->-1/2 1/2, 3->1/2 -1/2, 4->1/2 1/2}
    ITensor nnhT(mpo2s.Is1, prime(mpo2s.Is1), mpo2s.Is2, prime(mpo2s.Is2));
    nnhT.set(mpo2s.Is1(1), prime(mpo2s.Is1)(1),
    	mpo2s.Is2(1), prime(mpo2s.Is2)(1), exp(-tau*(J/4.0-h/z)) );

    nnhT.set(mpo2s.Is1(1), prime(mpo2s.Is1)(1),
    	mpo2s.Is2(2), prime(mpo2s.Is2)(2), exp(tau*J/4.0)*cosh(tau*J/2.0) );
    nnhT.set(mpo2s.Is1(1), prime(mpo2s.Is1)(2),
    	mpo2s.Is2(2), prime(mpo2s.Is2)(1), -exp(tau*J/4.0)*sinh(tau*J/2.0) );
    nnhT.set(mpo2s.Is1(2), prime(mpo2s.Is1)(1),
    	mpo2s.Is2(1), prime(mpo2s.Is2)(2), -exp(tau*J/4.0)*sinh(tau*J/2.0) );
    nnhT.set(mpo2s.Is1(2), prime(mpo2s.Is1)(2),
    	mpo2s.Is2(1), prime(mpo2s.Is2)(1), exp(tau*J/4.0)*cosh(tau*J/2.0) );

    nnhT.set(mpo2s.Is1(2), prime(mpo2s.Is1)(2),
    	mpo2s.Is2(2), prime(mpo2s.Is2)(2), exp(-tau*(J/4.0+h/z)) );

    PrintData(nnhT);

    /*
     *  s1'                    s2' 
     *   |                      |
     *  |H1|--a1--<SV_12>--a2--|H2|
     *   |                      |
     *  s1                     s2
     *
     */
    mpo2s.H1 = ITensor(mpo2s.Is1,prime(mpo2s.Is1));
    ITensor SV_12;
    svd(nnhT, mpo2s.H1,SV_12,mpo2s.H2);

    PrintData(mpo2s.H1);
    PrintData(SV_12);
    Print(mpo2s.H2);

    Index a1 = commonIndex(mpo2s.H1,SV_12);
    Index a2 = commonIndex(SV_12,mpo2s.H2);

    // Define aux indices linking the on-site MPOs
	Index iMPO3s12(TAG_MPO3S_12LINK,a1.m(),MPOLINK);

	/*
	 * Split obtained SVD values symmetricaly and absorb to obtain
	 * final tensor H1 and intermediate tensor temp
	 *
     *  s1'                                     s2' 
     *   |                                       |
     *  |H1|--a1--<SV_12>^1/2--<SV_12>^1/2--a2--|H2|
     *   |                                       |
     *  s1                                      s2
     *
     */
    SV_12.apply(sqrt_T);
    mpo2s.H1 = ( mpo2s.H1 * SV_12 )*delta(a2,iMPO3s12);
    mpo2s.H2 = ( mpo2s.H2 * SV_12 )*delta(a1,iMPO3s12);
    
    // ----- analyze signs of largest elements in H1, H2 and switch to 
    // positive sign -1 * -1 => +1 * +1 --------------------------------
 //    double m1, m2;
	// double m = 0.;
	// double sign = 0.;
 //    auto max_m = [&m, &sign](double d) {
 //        if(std::abs(d) > m) {
 //        	sign = (d > 0) ? 1 : ((d < 0) ? -1 : 0);
 //         	m = std::abs(d);
 //        }
 //    };

 //    mpo2s.H1.visit(max_m);
 //    m1 = m*sign;
 //    m = 0.;
 //    mpo2s.H2.visit(max_m);
 //    m2 = m*sign;

 //    std::cout <<"m1: "<< m1 <<" m2: "<< m2 << std::endl;

	// if ( m1*m2 > 0 ) {
	// 	mpo2s.H1 /= m1;
	// 	mpo2s.H2 /= m2;
	// }
	// ----- end sign analysis ----------------------------------------

    PrintData(mpo2s.H1);
    PrintData(mpo2s.H2);

	return mpo2s;
}

MPO_2site getMPO2s_NNHstagh(int z, double tau, double J, double h) {
	MPO_2site mpo2s;
	int physDim = 2; // dimension of Hilbert space of spin s=1/2 DoF
	
	// Define physical indices
	mpo2s.Is1 = Index(TAG_MPO3S_PHYS1,physDim,PHYS);
	mpo2s.Is2 = Index(TAG_MPO3S_PHYS2,physDim,PHYS);

	//create a lambda function
	//which returns the square of its argument
	auto sqrt_T = [](double r) { return sqrt(r); };

	// Define the individual tensor elements
	// The values of index enumerate local spin s=1/2 basis as follows
	// Is1 AND Is2 {1 -> s_z=-1/2, 2 -> s_z=1/2}
    // Thus tensor product of indices Is1(x)Is2 gives
    // {1->-1/2 -1/2, 2->-1/2 1/2, 3->1/2 -1/2, 4->1/2 1/2}
    ITensor nnhT(mpo2s.Is1, prime(mpo2s.Is1), mpo2s.Is2, prime(mpo2s.Is2));
    nnhT.set(mpo2s.Is1(1), prime(mpo2s.Is1)(1),
    	mpo2s.Is2(1), prime(mpo2s.Is2)(1), exp(-tau*J/4.0) );

    double arg = tau*sqrt(J*J/16.0 + h*h/(4.0*z*z));
    nnhT.set(mpo2s.Is1(1), prime(mpo2s.Is1)(1),
    	mpo2s.Is2(2), prime(mpo2s.Is2)(2), exp(tau*J/4.0)*(cosh(2.0*arg)-(0.5*h/z)*sinh(2.0*arg)/arg) );
    nnhT.set(mpo2s.Is1(1), prime(mpo2s.Is1)(2),
    	mpo2s.Is2(2), prime(mpo2s.Is2)(1), exp(tau*J/4.0)*(-J/4.0)*sinh(2.0*arg)/arg );
    nnhT.set(mpo2s.Is1(2), prime(mpo2s.Is1)(1),
    	mpo2s.Is2(1), prime(mpo2s.Is2)(2), exp(tau*J/4.0)*(-J/4.0)*sinh(2.0*arg)/arg ) ;
    nnhT.set(mpo2s.Is1(2), prime(mpo2s.Is1)(2),
    	mpo2s.Is2(1), prime(mpo2s.Is2)(1), exp(tau*J/4.0)*(cosh(2.0*arg)-(0.5*h/z)*sinh(2.0*arg)/arg) );

    nnhT.set(mpo2s.Is1(2), prime(mpo2s.Is1)(2),
    	mpo2s.Is2(2), prime(mpo2s.Is2)(2), exp(-tau*J/4.0) );

    PrintData(nnhT);

    /*
     *  s1'                    s2' 
     *   |                      |
     *  |H1|--a1--<SV_12>--a2--|H2|
     *   |                      |
     *  s1                     s2
     *
     */
    mpo2s.H1 = ITensor(mpo2s.Is1,prime(mpo2s.Is1));
    ITensor SV_12;
    svd(nnhT, mpo2s.H1,SV_12,mpo2s.H2);

    PrintData(mpo2s.H1);
    PrintData(SV_12);
    Print(mpo2s.H2);

    Index a1 = commonIndex(mpo2s.H1,SV_12);
    Index a2 = commonIndex(SV_12,mpo2s.H2);

    // Define aux indices linking the on-site MPOs
	Index iMPO3s12(TAG_MPO3S_12LINK,a1.m(),MPOLINK);

	/*
	 * Split obtained SVD values symmetricaly and absorb to obtain
	 * final tensor H1 and intermediate tensor temp
	 *
     *  s1'                                     s2' 
     *   |                                       |
     *  |H1|--a1--<SV_12>^1/2--<SV_12>^1/2--a2--|H2|
     *   |                                       |
     *  s1                                      s2
     *
     */
    SV_12.apply(sqrt_T);
    mpo2s.H1 = ( mpo2s.H1 * SV_12 )*delta(a2,iMPO3s12);
    mpo2s.H2 = ( mpo2s.H2 * SV_12 )*delta(a1,iMPO3s12);
    
    // ----- analyze signs of largest elements in H1, H2 and switch to 
    // positive sign -1 * -1 => +1 * +1 --------------------------------
 //    double m1, m2;
	// double m = 0.;
	// double sign = 0.;
 //    auto max_m = [&m, &sign](double d) {
 //        if(std::abs(d) > m) {
 //        	sign = (d > 0) ? 1 : ((d < 0) ? -1 : 0);
 //         	m = std::abs(d);
 //        }
 //    };

 //    mpo2s.H1.visit(max_m);
 //    m1 = m*sign;
 //    m = 0.;
 //    mpo2s.H2.visit(max_m);
 //    m2 = m*sign;

 //    std::cout <<"m1: "<< m1 <<" m2: "<< m2 << std::endl;

	// if ( m1*m2 > 0 ) {
	// 	mpo2s.H1 /= m1;
	// 	mpo2s.H2 /= m2;
	// }
	// ----- end sign analysis ----------------------------------------

    PrintData(mpo2s.H1);
    PrintData(mpo2s.H2);

	return mpo2s;
}

void applyH_T1_L_T2_DBG(MPO_2site const& mpo2s, 
	ITensor & T1, ITensor & T2, ITensor & L) {
	std::cout <<">>>>> applyH_12_T1_L_T2 called <<<<<"<< std::endl;
	std::cout << mpo2s;
	PrintData(mpo2s.H1);
    PrintData(mpo2s.H2);

	/*
	 * Applying 2-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               
	 *   --|1|~~|H1|~~s1           
	 *      |     |               s1       s2
	 *      L     |               |_       |_ 
	 *    \ |     |       ==   --|  |-----|  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|     |2~|--  
	 *      |             ==   --|__|--L--|__|--
     *
	 * Indices s1,s2 are relabeled back to physical indices of 
	 * original sites 1 and 2 after applying MPO.
	 *
	 */

	std::cout <<"----- Initial |12> -----"<< std::endl;
	Print(T1);
	Print(L);
	Print(T2);
	Index iT1_L = commonIndex(T1, L);
	Index iL_T2 = commonIndex(L, T2);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo2s.Is1);
	T1 = ( T1 * kd_phys1) * mpo2s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo2s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo2s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2 to |12> -----"<< std::endl;
	Print(T1);
    Print(T2);

	/*
	 * Perform SVD of new on-site tensors |1~| and |2~| by contrating them
	 * along diagonal matrix with weights
	 *
	 *       _______               s1                       s2
	 *  s1~~|       |~~s2           |                        |
	 *    --| 1~ 2~ |--    ==>  --|   |                    |   |--
	 *    --|       |--    ==>  --|1~~|++a1++|SV_L12|++a2++|2~~|--
	 *    --|_______|--         --|___|                    |___|--
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD and SV_L12 holds a new weights
	 * We keep only auxBondDim largest singular values
	 *
	 */

	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	ITensor SV_L12;
	svd(T1*L*T2, T1, SV_L12, T2, {"Maxm", iT1_L.m()});
	PrintData(SV_L12);

	// Set proper indices to resulting tensors from SVD routine
	Index iT1_SV_L12 = commonIndex(T1, SV_L12);
	Index iSV_L12_T2 = commonIndex(SV_L12, T2);

	T1 = T1 * delta(iT1_SV_L12, iT1_L);
	
	for (int i=1; i<=iT1_L.m(); i++) {
		L.set(iT1_L(i),iL_T2(i), SV_L12.real(iT1_SV_L12(i),iSV_L12_T2(i)));
	}
	L = L / norm(L);

	T2 = T2 * delta(iSV_L12_T2, iL_T2);

	Print(T1);
	PrintData(L);
	Print(T2);
}

void applyH_T1_L_T2(MPO_2site const& mpo2s, 
	ITensor & T1, ITensor & T2, ITensor & L) {

	/*
	 * Applying 2-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               
	 *   --|1|~~|H1|~~s1           
	 *      |     |               s1       s2
	 *      L     |               |_       |_ 
	 *    \ |     |       ==   --|  |-----|  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|     |2~|--  
	 *      |             ==   --|__|--L--|__|--
     *
	 * Indices s1,s2 are relabeled back to physical indices of 
	 * original sites 1 and 2 after applying MPO.
	 *
	 */
	Index iT1_L = commonIndex(T1, L);
	Index iL_T2 = commonIndex(L, T2);

	//std::cout <<"----- Appyling H1-H2 to |12> -----"<< std::endl;
	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo2s.Is1);
	T1 = ( T1 * kd_phys1) * mpo2s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo2s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo2s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);

	
	/*
	 * Perform SVD of new on-site tensors |1~| and |2~| by contrating them
	 * along diagonal matrix with weights
	 *
	 *       _______               s1                       s2
	 *  s1~~|       |~~s2           |                        |
	 *    --| 1~ 2~ |--    ==>  --|   |                    |   |--
	 *    --|       |--    ==>  --|1~~|++a1++|SV_L12|++a2++|2~~|--
	 *    --|_______|--         --|___|                    |___|--
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD and SV_L12 holds a new weights
	 * We keep only auxBondDim largest singular values
	 *
	 */
	//std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	ITensor SV_L12;
	svd(T1*L*T2, T1, SV_L12, T2, {"Maxm", iT1_L.m()});

	// Set proper indices to resulting tensors from SVD routine
	Index iT1_SV_L12 = commonIndex(T1, SV_L12);
	Index iSV_L12_T2 = commonIndex(SV_L12, T2);

	T1 = T1 * delta(iT1_SV_L12, iT1_L);
	
	for (int i=1; i<=iT1_L.m(); i++) {
		L.set(iT1_L(i),iL_T2(i), SV_L12.real(iT1_SV_L12(i),iSV_L12_T2(i)));
	}
	L = L / norm(L);

	T2 = T2 * delta(iSV_L12_T2, iL_T2);
}

void applyH_12(MPO_2site const& mpo2s, 
	ITensor & T1, ITensor & T2, 
	std::pair<Index, Index> const& link12) {

	std::cout <<">>>>> applyH_12_v1 called <<<<<"<< std::endl;
	std::cout << mpo2s;
	PrintData(mpo2s.H1);
    PrintData(mpo2s.H2);

	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;

	// Take the square-root of SV's
	auto sqrtT = [](double r) { return sqrt(r); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2
	 *   --|1|~~|H1|~~s1          |_    |_ 
	 *    \ |     |       ==   --|  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|--  
	 *      |     |       ==   --|__|  |__|--
              
	 *
	 * Indices s1,s2 are relabeled back to physical indices of 
	 * original sites 1 and 2 after applying MPO.
	 * Now auxiliary index linking sites 1 & 2 have dimension increased 
	 * from D to D*auxDim of applied 2-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along link12.
	 *
	 */

	std::cout <<"----- Initial |12> -----"<< std::endl;
	Print(T1);
	Print(T2);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo2s.Is1);
	T1 = ( T1 * kd_phys1) * mpo2s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo2s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo2s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2 to |12> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);

	/*
	 * Perform SVD of new on-site tensors |1~| and |2~| by contrating them
	 * along common index
	 *
	 *       _______               s1                       s2
	 *  s1~~|       |~~s2           |                        |
	 *    --| 1~ 2~ |--    ==>  --|   |                    |   |--
	 *    --|       |--    ==>  --|1~~|++a1++|SV_L12|++a2++|2~~|--
	 *    --|_______|--         --|___|                    |___|--
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	ITensor SV_L12;
	svd(T1*delta(link12.first, link12.second)*T2, T1, SV_L12, T2);

	Print(T1);
	PrintData(SV_L12);
	Print(T2);

	/*
	 * We absorb SV matrices into new on-site tensors in symmetric fashion,
	 * absorbing square root of SV matrix of the link to both on-site
	 * tensors which it connects. Then we discard the excess SV values
	 * to reduce back to the auxBond dimension D
	 *
	 * ++|SV_L12|++ => ++|SV_L12^1/2|++|SV_L12^1/2|++
	 *
	 *     s1
	 *     |
	 *  --|   |                                    ==>    \ |
	 *  --|1~~|++|SV_L12^1/2|++a2>>--link12.first  ==>  --|1n|~~s1
	 *  --|___|                                    ==>      |
	 *
	 *  where dR is a reduction tensor, which discards all the SV values
	 *  except the first D. Analogous operation is performed on the |2~~~|
	 *  on-site tensor
	 *
	 *                                       s2
     *                                       |       
	 *                                     |    |--  ==>   \ |
	 *  link12.second--<<a1++|SV_L12^1/2|++|2~~~|--  ==> --|2n|~~s2
	 *                                     |____|--  ==>     |
	 *                                       
	 */

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	SV_L12.apply(sqrtT);
	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(T2,SV_L12);
	T1 = ( T1*SV_L12 )*delta( a2, link12.first );
	T2 = ( T2*SV_L12 )*delta( a1, link12.second );

	PrintData(SV_L12);
	PrintData(T1);
	PrintData(T2);
}

void applyH_12_v2(MPO_2site const& mpo2s, 
	ITensor & T1, ITensor & T2, 
	std::pair<Index, Index> const& link12) {

	std::cout <<">>>>> applyH_12_v1 called <<<<<"<< std::endl;
	std::cout << mpo2s;
	PrintData(mpo2s.H1);
    PrintData(mpo2s.H2);

	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;

	// Take the square-root of SV's
	auto sqrtT = [](double r) { return sqrt(r); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2
	 *   --|1|~~|H1|~~s1          |_    |_ 
	 *    \ |     |       ==   --|  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|--  
	 *      |     |       ==   --|__|  |__|--
              
	 *
	 * Indices s1,s2 are relabeled back to physical indices of 
	 * original sites 1 and 2 after applying MPO.
	 * Now auxiliary index linking sites 1 & 2 have dimension increased 
	 * from D to D*auxDim of applied 2-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along link12.
	 *
	 */

	std::cout <<"----- Initial |12> -----"<< std::endl;
	Print(T1);
	Print(T2);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo2s.Is1);
	T1 = ( T1 * kd_phys1) * mpo2s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo2s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo2s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2 to |12> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);

	/*
	 * Perform SVD of new on-site tensors |1~| and |2~| by contrating them
	 * along common index
	 *
	 *       _______               s1                       s2
	 *  s1~~|       |~~s2           |                        |
	 *    --| 1~ 2~ |--    ==>  --|   |                    |   |--
	 *    --|       |--    ==>  --|1~~|++a1++|SV_L12|++a2++|2~~|--
	 *    --|_______|--         --|___|                    |___|--
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	ITensor SV_L12;
	svd(T1*delta(link12.first, link12.second)*T2, T1, SV_L12, T2);

	Print(T1);
	PrintData(SV_L12);
	Print(T2);

	/*
	 * We absorb SV matrices into new on-site tensors in asymmetric fashion,
	 * absorbing SV matrix of the link to, say, H1.
	 * Then we discard the excessive SV values to reduce back to the auxBond
	 * dimension D
	 *
	 *     s1
	 *     |
	 *  --|   |                                  ==>    \ |
	 *  --|1~~|++|SV_L12^|++a2>>link12.first     ==>  --|1n|~~s1
	 *  --|___|                                  ==>      |
	 *
     * On the |2~~~| on-site tensor we reduce the dimension of aux index
	 * back to D
	 *                                       s2
     *                                       |       
	 *                                     |    |--  ==>   \ |
	 *                  link12.second<<a1++|2~~~|--  ==> --|2n|~~s2
	 *                                     |____|--  ==>     |
	 *                                       
	 */

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(T2,SV_L12);
	T1 = ( T1*SV_L12 )*delta( a2, link12.first );
	T2 = T2*delta( a2, link12.second );

	PrintData(SV_L12);
	PrintData(T1);
	PrintData(T2);
}

// 3 SITE OPS #########################################################

MPO_3site getMPO3s_Id(int physDim) {
	MPO_3site mpo3s;
	
	// Define physical indices
	mpo3s.Is1 = Index(TAG_MPO3S_PHYS1,physDim,PHYS);
	mpo3s.Is2 = Index(TAG_MPO3S_PHYS2,physDim,PHYS);
	mpo3s.Is3 = Index(TAG_MPO3S_PHYS3,physDim,PHYS);

	//create a lambda function
	//which returns the square of its argument
	auto sqrt_T = [](double r) { return sqrt(r); };

    ITensor idpI1(mpo3s.Is1,prime(mpo3s.Is1,1));
    ITensor idpI2(mpo3s.Is2,prime(mpo3s.Is2,1));
    ITensor idpI3(mpo3s.Is3,prime(mpo3s.Is3,1));
    for (int i=1;i<=physDim;i++) {
        idpI1.set(mpo3s.Is1(i),prime(mpo3s.Is1,1)(i),1.0);
        idpI2.set(mpo3s.Is2(i),prime(mpo3s.Is2,1)(i),1.0);
        idpI3.set(mpo3s.Is3(i),prime(mpo3s.Is3,1)(i),1.0);
    }

    ITensor id3s = idpI1*idpI2*idpI3;

    /*
     *  s1'                    s2' s3' 
     *   |                      |  |
     *  |H1|--a1--<SV_12>--a2--|temp|
     *   |                      |  |
     *  s1                     s2  s3
     *
     */
    mpo3s.H1 = ITensor(mpo3s.Is1,prime(mpo3s.Is1));
    ITensor SV_12,temp;
    svd(id3s,mpo3s.H1,SV_12,temp);

    PrintData(mpo3s.H1);
    PrintData(SV_12);
    Print(temp);

    Index a1 = commonIndex(mpo3s.H1,SV_12);
    Index a2 = commonIndex(SV_12,temp);

    // Define aux indices linking the on-site MPOs
	Index iMPO3s12(TAG_MPO3S_12LINK,a1.m(),MPOLINK);

	/*
	 * Split obtained SVD values symmetricaly and absorb to obtain
	 * final tensor H1 and intermediate tensor temp
	 *
     *  s1'                                     s2' s3' 
     *   |                                       |  |
     *  |H1|--a1--<SV_12>^1/2--<SV_12>^1/2--a2--|temp|
     *   |                                       |  |
     *  s1                                      s2  s3
     *
     */
    SV_12.apply(sqrt_T);
    mpo3s.H1 = ( mpo3s.H1 * SV_12 )*delta(a2,iMPO3s12);
    temp = ( temp * SV_12 )*delta(a1,iMPO3s12);
    Print(mpo3s.H1);
    Print(temp);
    
    /*
     *  s1'    s2'                    s3' 
     *   |     |                      |
     *  |H1|--|H2|--a3--<SV_23>--a4--|H3|
     *   |     |                      |
     *  s1     s2                     s3
     *
     */
	mpo3s.H2 = ITensor(mpo3s.Is2,prime(mpo3s.Is2,1),iMPO3s12);
	ITensor SV_23;
    svd(temp,mpo3s.H2,SV_23,mpo3s.H3);

    Print(mpo3s.H2);
    PrintData(SV_23);
    Print(mpo3s.H3);

	Index a3 = commonIndex(mpo3s.H2,SV_23);
	Index a4 = commonIndex(SV_23,mpo3s.H3);
  
	/*
	 *cSplit obtained SVD values symmetricaly and absorb to obtain
	 * final tensor H1 and H3
	 *
     *  s1'    s2'                                     s3' 
     *   |     |                                       |
     *  |H1|--|H2|--a3--<SV_23>^1/2--<SV_23>^1/2--a4--|H3|
     *   |     |                                       |
     *  s1     s2                                      s3
     *
     */
	SV_23.apply(sqrt_T);
	Index iMPO3s23(TAG_MPO3S_23LINK,a3.m(),MPOLINK);
	mpo3s.H2 = (mpo3s.H2 * SV_23)*delta(a4,iMPO3s23);
	mpo3s.H3 = (mpo3s.H3 * SV_23)*delta(a3,iMPO3s23);

	PrintData(mpo3s.H1);
	PrintData(mpo3s.H2);
	PrintData(mpo3s.H3);

	return mpo3s;
}

MPO_3site getMPO3s_Id_v2(int physDim) {
	MPO_3site mpo3s;
	
	// Define physical indices
	mpo3s.Is1 = Index(TAG_MPO3S_PHYS1,physDim,PHYS);
	mpo3s.Is2 = Index(TAG_MPO3S_PHYS2,physDim,PHYS);
	mpo3s.Is3 = Index(TAG_MPO3S_PHYS3,physDim,PHYS);

	//create a lambda function
	//which returns the square of its argument
	auto sqrt_T = [](double r) { return sqrt(r); };
	double pw;
	auto pow_T = [&pw](double r) { return std::pow(r,pw); };

    ITensor idpI1(mpo3s.Is1,prime(mpo3s.Is1,1));
    ITensor idpI2(mpo3s.Is2,prime(mpo3s.Is2,1));
    ITensor idpI3(mpo3s.Is3,prime(mpo3s.Is3,1));
    for (int i=1;i<=physDim;i++) {
        idpI1.set(mpo3s.Is1(i),prime(mpo3s.Is1,1)(i),1.0);
        idpI2.set(mpo3s.Is2(i),prime(mpo3s.Is2,1)(i),1.0);
        idpI3.set(mpo3s.Is3(i),prime(mpo3s.Is3,1)(i),1.0);
    }

    ITensor id3s = idpI1*idpI2*idpI3;
    //PrintData(id3s);

    mpo3s.H1 = ITensor(mpo3s.Is1,prime(mpo3s.Is1));
    ITensor SV_12,temp;
    svd(id3s,mpo3s.H1,SV_12,temp);
    
    PrintData(mpo3s.H1);
    PrintData(SV_12);
    Print(temp);

    /*
     *  s1'                    s2' s3' 
     *   |                      |  |
     *  |H1|--a1--<SV_12>--a2--|temp|
     *   |                      |  |
     *  s1                     s2  s3
     *
     */
    Index a1 = commonIndex(mpo3s.H1,SV_12);
    Index a2 = commonIndex(SV_12,temp);

    // Define aux indices linking the on-site MPOs
	Index iMPO3s12(TAG_MPO3S_12LINK,a1.m(),MPOLINK);
	
	pw = 2.0/3.0;
	SV_12.apply(pow_T); // x^2/3
	PrintData(SV_12);
	temp = ( temp * SV_12 )*delta(a1,iMPO3s12);
	Print(temp);

	pw = 1.0/2.0; 
	SV_12.apply(pow_T); // x^(2/3*1/2) = x^1/3
    mpo3s.H1 = ( mpo3s.H1 * SV_12 )*delta(a2,iMPO3s12);
    PrintData(mpo3s.H1);
    
	mpo3s.H2 = ITensor(mpo3s.Is2,prime(mpo3s.Is2,1),iMPO3s12);
	ITensor SV_23;
    svd(temp,mpo3s.H2,SV_23,mpo3s.H3);

    PrintData(mpo3s.H1);
    PrintData(SV_12);
    PrintData(mpo3s.H2);
    PrintData(SV_23);
    PrintData(mpo3s.H3);

    /*
     *  s1'                    s2'                    s3' 
     *   |                      |                      |
     *  |H1|--a1--<SV_12>--a2--|H2|--a3--<SV_23>--a4--|H3|
     *   |                      |                      |
     *  s1                     s2                     s3
     *
     */
	SV_23.apply(sqrt_T);
	
	Index a3 = commonIndex(mpo3s.H2,SV_23);
	Index a4 = commonIndex(SV_23,mpo3s.H3);
	Index iMPO3s23(TAG_MPO3S_23LINK,a3.m(),MPOLINK);
	mpo3s.H2 = (mpo3s.H2 * SV_23)*delta(a4,iMPO3s23);
	mpo3s.H3 = (mpo3s.H3 * SV_23)*delta(a3,iMPO3s23);

	double m1, m2, m3;
	double sign1, sign2, sign3;
	double m = 0.;
	double sign = 0.;
    auto max_m = [&m, &sign](double d) {
        if(std::abs(d) > m) {
        	sign = (d > 0) ? 1 : ((d < 0) ? -1 : 0);
         	m = std::abs(d);
        }
    };

    mpo3s.H1.visit(max_m);
    sign1 = sign;
    m1 = m*sign;
    m = 0.;
    mpo3s.H2.visit(max_m);
    m2 = m*sign;
    m = 0.;
    mpo3s.H3.visit(max_m);
    m3 = m*sign;

    std::cout <<"m1: "<< m1 <<" m2: "<< m2 <<" m3: "<< m3 << std::endl;

	if ( m1*m2*m3 > 0 ) {
		mpo3s.H1 /= m1;
		mpo3s.H2 /= m2;
		mpo3s.H3 /= m3;	
	}
	 
	PrintData(mpo3s.H1);
	PrintData(mpo3s.H2);
	PrintData(mpo3s.H3);

	return mpo3s;
}

void applyH_123(MPO_3site const& mpo3s, 
	ITensor & T1, ITensor & T2, ITensor & T3, 
	std::pair<Index, Index> const& link12, 
	std::pair<Index, Index> const& link23) {

	std::cout <<">>>>> applyH_123_v1 called <<<<<"<< std::endl;
	std::cout << mpo3s;
	PrintData(mpo3s.H1);
    PrintData(mpo3s.H2);
    PrintData(mpo3s.H3);

	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;
	std::cout <<"link23 "<< link23.first <<" "<< link23.second << std::endl;

	// Take the square-root of SV's
	auto sqrtT = [](double r) { return sqrt(r); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2    s3
	 *   --|1|~~|H1|~~s1          |_    |_    |_
	 *    \ |     |       ==   --|  |  |  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|==|3~|--  
	 *    \ |     |       ==   --|__|  |__|  |__|--
	 *   --|3|~~|H3|~~s3                ||
	 *      |                       
	 *
	 * Indices s1,s2,s3 are relabeled back to physical indices of 
	 * original sites 1,2 and 3 after applying MPO.
	 * Now auxiliary index linking sites 1,2 & 3 have dimension increased 
	 * from D to D*auxDim of applied 3-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along links 12 and 23.
	 *
	 */

	std::cout <<"----- Initial |123> -----"<< std::endl;
	Print(T1);
	Print(T2);
	Print(T3);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo3s.Is1);
	T1 = ( T1 * kd_phys1) * mpo3s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo3s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo3s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys3 = delta(findtype(T3.inds(),PHYS), mpo3s.Is3);
	T3 = ( T3 * kd_phys3 ) * mpo3s.H3;
	T3 = (T3 * kd_phys3.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2-H3 to |123> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);
    PrintData(T3);

	/*
	 * Perform SVD of new on-site tensors |1~| and |2~| by contrating them
	 * along common index
	 *
	 *       _______               s1                       s2
	 *  s1~~|       |~~s2           |                        |
	 *    --| 1~ 2~ |==    ==>  --|   |                    |   |
	 *    --|       |--    ==>  --|1~~|++a1++|SV_L12|++a2++|2~~|==
	 *    --|_______|--         --|___|                    |___|
	 *                                                      ||
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	ITensor SV_L12;
	svd(T1*delta(link12.first, link12.second)*T2, T1, SV_L12, T2);

	Print(T1);
	PrintData(SV_L12);
	Print(T2);

	/*
	 * Perform SVD of new on-site tensors |2~~| and |3~| by contrating them
	 * along common index
	 *
	 *       _______               s2                         s3
	 *  s2~~|       |~~s3           |                         |
	 *    ++| 2~~ 3~|--    ==>    |    |                    |   |--
	 *    --|       |--    ==>  ++|2~~~|++a3++|SV_L23|++a4++|3~~|--
	 *    --|_______|--           |____|                    |___|--
	 *                              ||
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link23 -----"<< std::endl;
	ITensor SV_L23;
	svd(T2*delta(link23.first, link23.second)*T3, T3, SV_L23, T2);

	Print(T2);
	PrintData(SV_L23);
	Print(T3);

	/*
	 * We absorb SV matrices into new on-site tensors in symmetric fashion,
	 * absorbing square root of SV matrix of the link to both on-site
	 * tensors which it connects. Then we discard the excess SV values
	 * to reduce back to the auxBond dimension D
	 *
	 * ++|SV_L12|++ => ++|SV_L12^1/2|++|SV_L12^1/2|++
	 *
	 *     s1
	 *     |
	 *  --|   |                                  ==>    \ |
	 *  --|1~~|++|SV_L12^1/2|++a2>>--link12.first  ==>  --|1n|~~s1
	 *  --|___|                                  ==>      |
	 *
	 *  where dR is a reduction tensor, which discards all the SV values
	 *  except the first D. Analogous operation is performed on the |2~~~|
	 *  on-site tensor
	 *
	 *                                       s2
     *                                       |       
	 *                                     |    |    ==>   \ |
	 *  link12.second--<<a1++|SV_L12^1/2|++|2~~~|++  ==> --|2n|~~s2
	 *                                     |____|    ==>     |
	 *                                       ||
	 */

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	SV_L12.apply(sqrtT);
	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(T2,SV_L12);
	T1 = ( T1*SV_L12 )*delta( a2, link12.first );
	T2 = ( T2*SV_L12 )*delta( a1, link12.second );

	PrintData(SV_L12);
	PrintData(T1);
	Print(T2);

	std::cout <<"----- (NOT)Reducing dimension of link23 -----"<< std::endl;
	SV_L23.apply(sqrtT);
	auto a3 = commonIndex(T2,SV_L23);
	auto a4 = commonIndex(T3,SV_L23);
	T2 = ( T2*SV_L23 )*delta( a4, link23.first );
	T3 = ( T3*SV_L23 )*delta( a3, link23.second );

	PrintData(SV_L23);
	PrintData(T2);
	PrintData(T3);
}

void applyH_123_v2(MPO_3site const& mpo3s, 
	ITensor & T1, ITensor & T2, ITensor & T3, 
	std::pair<Index, Index> const& link12, 
	std::pair<Index, Index> const& link23) {

	std::cout <<">>>>> applyH_123_v2 called <<<<<"<< std::endl;
	std::cout << mpo3s;
	PrintData(mpo3s.H1);
    PrintData(mpo3s.H2);
    PrintData(mpo3s.H3);

	int auxBondDim = link12.first.m();
	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;
	std::cout <<"link23 "<< link23.first <<" "<< link23.second << std::endl;

	// Take the square-root of SV's
	auto sqrtT = [](double r) { return sqrt(r); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2    s3
	 *   --|1|~~|H1|~~s1          |_    |_    |_
	 *    \ |     |       ==   --|  |  |  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|==|3~|--  
	 *    \ |     |       ==   --|__|  |__|  |__|--
	 *   --|3|~~|H3|~~s3                ||
	 *      |                       
	 *
	 * Indices s1,s2,s3 are relabeled back to physical indices of 
	 * original sites 1,2 and 3 after applying MPO.
	 * Now auxiliary index linking sites 1,2 & 3 have dimension increased 
	 * from D to D*auxDim of applied 3-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along links 12 and 23.
	 *
	 */

	std::cout <<"----- Initial |123> -----"<< std::endl;
	Print(T1);
	Print(T2);
	Print(T3);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo3s.Is1);
	T1 = ( T1 * kd_phys1) * mpo3s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo3s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo3s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys3 = delta(findtype(T3.inds(),PHYS), mpo3s.Is3);
	T3 = ( T3 * kd_phys3 ) * mpo3s.H3;
	T3 = (T3 * kd_phys3.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2-H3 to |123> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);
    PrintData(T3);

	/*
	 * Obtain new on-site tensors |1~~| by contrating tensors 1~,2~ and 3~
	 * along common indices
	 *
	 *       _________                  s1              s2 s3 
	 *  s1~~|         |~~s2            _|_              _|__|_
	 *    --| 1~ 2~ 3~|~~s3    ==>  --|   |            |      |--
	 *    --|         |--      ==>  --|1~~|++|SV_L12|++|2~3~  |--
	 *    --|_________|--           --|___|            |______|--
	 *          | |                                     | |
	 *
	 * where 1~~ and 2~3~ are now holding singular vectors wrt
	 * to SVD done along "link" between sites 1 and "rest"
	 *
	 */
	ITensor temp = ((( T1*delta(link12.first, link12.second) )*T2 )
		*delta(link23.first, link23.second) )*T3;

	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	
	ITensor SV_L12, temp2;
	Args args = Args::global();
    args.add("Maxm", auxBondDim);
	auto spec = svd(temp, T1, SV_L12, temp2, args);
	Print(spec);

	Print(T1);
	PrintData(SV_L12);
	Print(temp2);

	/*
	 * Perform SVD of new on-site tensors |2~~| and |3~| by contrating them
	 * along common index
	 *
	 *       ______               s2               s3
	 *  s2~~|      |~~s3           |                |
	 *    ++| 2~ 3~|--    ==>    |    |            |   |--
	 *      |      |--    ==>  ++|2~~ |++|SV_L23|++|3~~|--
	 *      |______|--           |____|            |___|--
	 *       | |                   | |
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link23 -----"<< std::endl;
	ITensor SV_L23;
	spec = svd(temp2, T3, SV_L23, T2, args);
	Print(spec);

	Print(T2);
	PrintData(SV_L23);
	Print(T3);

	/*
	 * We absorb SV matrices into new on-site tensors in symmetric fashion,
	 * absorbing square root of SV matrix of the link to both on-site
	 * tensors which it connects. Then we discard the excess SV values
	 * to reduce back to the auxBond dimension D
	 *
	 * ++|SV_L12|++ => ++|SV_L12^1/2|++|SV_L12^1/2|++
	 *
	 *     s1
	 *     |
	 *  --|   |                                     ==>    \ |
	 *  --|1~~|++|SV_L12^1/2|++|dR1|--link12.first  ==>  --|1n|~~s1
	 *  --|___|                                     ==>      |
	 *
	 *  where dR is a reduction tensor, which discards all the SV values
	 *  except the first D. Analogous operation is performed on the |2~~~|
	 *  on-site tensor
	 *
	 *                                        s2
     *                                        |       
	 *                                      |    |    ==>   \ |
	 *  link12.second--|dR2|++|SV_L12^1/2|++|2~~~|++  ==> --|2n|~~s2
	 *                                      |____|    ==>     |
	 *                                        ||
	 */

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	SV_L12.apply(sqrtT);
	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(T2,SV_L12);
	T1 = ( T1*SV_L12 )*delta( a2, link12.first );
	T2 = ( T2*SV_L12 )*delta( a1, link12.second );

	PrintData(SV_L12);
	PrintData(T1);
	Print(T2);

	std::cout <<"----- (NOT)Reducing dimension of link23 -----"<< std::endl;
	SV_L23.apply(sqrtT);
	auto a3 = commonIndex(T2,SV_L23);
	auto a4 = commonIndex(T3,SV_L23);
	T2 = ( T2*SV_L23 )*delta( a4, link23.first );
	T3 = ( T3*SV_L23 )*delta( a3, link23.second );

	PrintData(SV_L23);
	PrintData(T2);
	PrintData(T3);
}

void applyH_123_v3(MPO_3site const& mpo3s, 
	ITensor & T1, ITensor & T2, ITensor & T3, 
	std::pair<Index, Index> const& link12, 
	std::pair<Index, Index> const& link23) {

	std::cout <<">>>>> applyH_123_v3 called <<<<<"<< std::endl;
	std::cout << mpo3s;
	PrintData(mpo3s.H1);
    PrintData(mpo3s.H2);
    PrintData(mpo3s.H3);

    int auxBondDim = link12.first.m();
	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;
	std::cout <<"link23 "<< link23.first <<" "<< link23.second << std::endl;

	// Take the square-root of SV's
	auto sqrtT = [](double r) { return sqrt(r); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2    s3
	 *   --|1|~~|H1|~~s1          |_    |_    |_
	 *    \ |     |       ==   --|  |  |  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|==|3~|--  
	 *    \ |     |       ==   --|__|  |__|  |__|--
	 *   --|3|~~|H3|~~s3                ||
	 *      |                       
	 *
	 * Indices s1,s2,s3 are relabeled back to physical indices of 
	 * original sites 1,2 and 3 after applying MPO.
	 * Now auxiliary index linking sites 1,2 & 3 have dimension increased 
	 * from D to D*auxDim of applied 3-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along links 12 and 23.
	 *
	 */

	std::cout <<"----- Initial |123> -----"<< std::endl;
	Print(T1);
	Print(T2);
	Print(T3);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo3s.Is1);
	T1 = ( T1 * kd_phys1) * mpo3s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo3s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo3s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys3 = delta(findtype(T3.inds(),PHYS), mpo3s.Is3);
	T3 = ( T3 * kd_phys3 ) * mpo3s.H3;
	T3 = (T3 * kd_phys3.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2-H3 to |123> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);
    PrintData(T3);

	/*
	 * Obtain new on-site tensors |1~~| by contrating tensors 1~,2~ and 3~
	 * along common indices
	 *
	 *       _________                  s1              s2 s3 
	 *  s1~~|         |~~s2            _|_              _|__|_
	 *    --| 1~ 2~ 3~|~~s3    ==>  --|   |            |      |--
	 *    --|         |--      ==>  --|1~~|++|SV_L12|++|2~3~  |--
	 *    --|_________|--           --|___|            |______|--
	 *          | |                                     | |
	 *
	 * where 1~~ and 2~3~ are now holding singular vectors wrt
	 * to SVD done along "link" between sites 1 and "rest"
	 *
	 */
	ITensor temp = ((( T1*delta(link12.first, link12.second) )*T2 )
		*delta(link23.first, link23.second) )*T3;

	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	
	ITensor SV_L12,temp2;
	Args args = Args::global();
    args.add("Maxm", auxBondDim);
	auto spec = svd(temp, T1, SV_L12, temp2, args);
	Print(spec);

	Print(T1);
	PrintData(SV_L12);
	Print(temp2);

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	SV_L12.apply(sqrtT);
	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(temp2,SV_L12);
	T1 = ( T1*SV_L12 )*delta( a2, link12.first );
	temp2 = ( temp2*SV_L12 )*delta( a1, link12.second );

	PrintData(T1);
	PrintData(SV_L12);
	Print(temp2);

	/*
	 * Perform SVD of new on-site tensors |2~~| and |3~| by contrating them
	 * along common index
	 *
	 *       ______               s2               s3
	 *  s2~~|      |~~s3           |                |
	 *    ++| 2~ 3~|--    ==>    |    |            |   |--
	 *      |      |--    ==>  ++|2~~ |++|SV_L23|++|3~~|--
	 *      |______|--           |____|            |___|--
	 *       | |                   | |
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link23 -----"<< std::endl;
	ITensor SV_L23;
	spec = svd(temp2, T3, SV_L23, T2, args);
	Print(spec);

	Print(T2);
	PrintData(SV_L23);
	Print(T3);

	/*
	 * We absorb SV matrices into new on-site tensors in symmetric fashion,
	 * absorbing square root of SV matrix of the link to both on-site
	 * tensors which it connects. Then we discard the excess SV values
	 * to reduce back to the auxBond dimension D
	 *
	 * ++|SV_L12|++ => ++|SV_L12^1/2|++|SV_L12^1/2|++
	 *
	 *     s1
	 *     |
	 *  --|   |                                     ==>    \ |
	 *  --|1~~|++|SV_L12^1/2|++|dR1|--link12.first  ==>  --|1n|~~s1
	 *  --|___|                                     ==>      |
	 *
	 *  where dR is a reduction tensor, which discards all the SV values
	 *  except the first D. Analogous operation is performed on the |2~~~|
	 *  on-site tensor
	 *
	 *                                        s2
     *                                        |       
	 *                                      |    |    ==>   \ |
	 *  link12.second--|dR2|++|SV_L12^1/2|++|2~~~|++  ==> --|2n|~~s2
	 *                                      |____|    ==>     |
	 *                                        ||
	 */

	std::cout <<"----- (NOT)Reducing dimension of link23 -----"<< std::endl;
	SV_L23.apply(sqrtT);
	auto a3 = commonIndex(T2,SV_L23);
	auto a4 = commonIndex(T3,SV_L23);
	T2 = ( T2*SV_L23 )*delta( a4, link23.first );
	T3 = ( T3*SV_L23 )*delta( a3, link23.second );

	PrintData(SV_L23);
	PrintData(T2);
	PrintData(T3);
}

void applyH_123_v4(MPO_3site const& mpo3s, 
	ITensor & T1, ITensor & T2, ITensor & T3, 
	std::pair<Index, Index> const& link12, 
	std::pair<Index, Index> const& link23) {

	std::cout <<">>>>> applyH_123_v4 called <<<<<"<< std::endl;
	std::cout << mpo3s;
	PrintData(mpo3s.H1);
    PrintData(mpo3s.H2);
    PrintData(mpo3s.H3);

    int auxBondDim = link12.first.m();
	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;
	std::cout <<"link23 "<< link23.first <<" "<< link23.second << std::endl;

	// Take the square-root of SV's
	auto sqrtT = [](double r) { return sqrt(r); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2    s3
	 *   --|1|~~|H1|~~s1          |_    |_    |_
	 *    \ |     |       ==   --|  |  |  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|==|3~|--  
	 *    \ |     |       ==   --|__|  |__|  |__|--
	 *   --|3|~~|H3|~~s3                ||
	 *      |                       
	 *
	 * Indices s1,s2,s3 are relabeled back to physical indices of 
	 * original sites 1,2 and 3 after applying MPO.
	 * Now auxiliary index linking sites 1,2 & 3 have dimension increased 
	 * from D to D*auxDim of applied 3-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along links 12 and 23.
	 *
	 */

	std::cout <<"----- Initial |123> -----"<< std::endl;
	Print(T1);
	Print(T2);
	Print(T3);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo3s.Is1);
	T1 = ( T1 * kd_phys1) * mpo3s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo3s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo3s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys3 = delta(findtype(T3.inds(),PHYS), mpo3s.Is3);
	T3 = ( T3 * kd_phys3 ) * mpo3s.H3;
	T3 = (T3 * kd_phys3.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2-H3 to |123> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);
    PrintData(T3);

	/*
	 * Obtain new on-site tensors |1~~| by contrating tensors 1~,2~ and 3~
	 * along common indices
	 *
	 *       _________                  s1              s2 s3 
	 *  s1~~|         |~~s2            _|_              _|__|_
	 *    --| 1~ 2~ 3~|~~s3    ==>  --|   |            |      |--
	 *    --|         |--      ==>  --|1~~|++|SV_L12|++|2~3~  |--
	 *    --|_________|--           --|___|            |______|--
	 *          | |                                     | |
	 *
	 * where 1~~ and 2~3~ are now holding singular vectors wrt
	 * to SVD done along "link" between sites 1 and "rest"
	 *
	 */
	ITensor temp = ((( T1*delta(link12.first, link12.second) )*T2 )
		*delta(link23.first, link23.second) )*T3;

	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	
	ITensor SV_L12,temp2;
	Args args = Args::global();
    args.add("Maxm", auxBondDim);
	auto spec = svd(temp, T1, SV_L12, temp2, args);
	Print(spec);

	Print(T1);
	PrintData(SV_L12);
	Print(temp2);

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(temp2,SV_L12);
	temp2 = ( temp2*SV_L12 );

	PrintData(SV_L12);
	PrintData(T1);
	Print(temp2);

	/*
	 * Perform SVD of new on-site tensors |2~~| and |3~| by contrating them
	 * along common index
	 *
	 *       ______               s2               s3
	 *  s2~~|      |~~s3           |                |
	 *    ++| 2~ 3~|--    ==>    |    |            |   |--
	 *      |      |--    ==>  ++|2~~ |++|SV_L23|++|3~~|--
	 *      |______|--           |____|            |___|--
	 *       | |                   | |
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link23 -----"<< std::endl;
	ITensor SV_L23;
	spec = svd(temp2, T3, SV_L23, T2, args);
	Print(spec);

	Print(T2);
	PrintData(SV_L23);
	Print(T3);

	/*
	 * We absorb SV matrices into new on-site tensors in symmetric fashion,
	 * absorbing square root of SV matrix of the link to both on-site
	 * tensors which it connects. Then we discard the excess SV values
	 * to reduce back to the auxBond dimension D
	 *
	 * ++|SV_L12|++ => ++|SV_L12^1/2|++|SV_L12^1/2|++
	 *
	 *     s1
	 *     |
	 *  --|   |                                     ==>    \ |
	 *  --|1~~|++|SV_L12^1/2|++|dR1|--link12.first  ==>  --|1n|~~s1
	 *  --|___|                                     ==>      |
	 *
	 *  where dR is a reduction tensor, which discards all the SV values
	 *  except the first D. Analogous operation is performed on the |2~~~|
	 *  on-site tensor
	 *
	 *                                        s2
     *                                        |       
	 *                                      |    |    ==>   \ |
	 *  link12.second--|dR2|++|SV_L12^1/2|++|2~~~|++  ==> --|2n|~~s2
	 *                                      |____|    ==>     |
	 *                                        ||
	 */

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	/*ITensor SV_L12_inv(a1,a2);
	for (int i=1;i<=a1.m();i++) {
		if( SV_L12.real(a1(i),a2(i)) >= 1.0e-10 ) {
			SV_L12_inv.set(a1(i), a2(i), 1.0/SV_L12.real(a1(i),a2(i)) );
		}
	}
	PrintData(SV_L12_inv);
	T2 = ( T2*SV_L12_inv )*delta(a2,a1);
	Print(T2);*/

	SV_L12.apply(sqrtT);	
	//auto a1 = commonIndex(T1,SV_L12);
	//auto a2 = commonIndex(T2,SV_L12);
	T1 = T1*delta( a1, link12.first );
	T2 = T2*delta( a1, link12.second );

	PrintData(SV_L12);
	PrintData(T1);
	Print(T2);

	std::cout <<"----- (NOT)Reducing dimension of link23 -----"<< std::endl;
	SV_L23.apply(sqrtT);
	auto a3 = commonIndex(T2,SV_L23);
	auto a4 = commonIndex(T3,SV_L23);
	T2 = ( T2*SV_L23 )*delta( a4, link23.first );
	T3 = ( T3*SV_L23 )*delta( a3, link23.second );

	PrintData(SV_L23);
	PrintData(T2);
	PrintData(T3);
}

void applyH_123_v5(MPO_3site const& mpo3s, 
	ITensor & T1, ITensor & T2, ITensor & T3, 
	std::pair<Index, Index> const& link12, 
	std::pair<Index, Index> const& link23) {

	std::cout <<">>>>> applyH_123_v5 called <<<<<"<< std::endl;
	std::cout << mpo3s; 
	PrintData(mpo3s.H1);
    PrintData(mpo3s.H2);
    PrintData(mpo3s.H3);

    int auxBondDim = link12.first.m();
	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;
	std::cout <<"link23 "<< link23.first <<" "<< link23.second << std::endl;

	// Take the square-root of SV's
	auto sqrtT = [](double r) { return sqrt(r); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2    s3
	 *   --|1|~~|H1|~~s1          |_    |_    |_
	 *    \ |     |       ==   --|  |  |  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|==|3~|--  
	 *    \ |     |       ==   --|__|  |__|  |__|--
	 *   --|3|~~|H3|~~s3                ||
	 *      |                       
	 *
	 * Indices s1,s2,s3 are relabeled back to physical indices of 
	 * original sites 1,2 and 3 after applying MPO.
	 * Now auxiliary index linking sites 1,2 & 3 have dimension increased 
	 * from D to D*auxDim of applied 3-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along links 12 and 23.
	 *
	 */

	std::cout <<"----- Initial |123> -----"<< std::endl;
	Print(T1);
	Print(T2);
	Print(T3);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo3s.Is1);
	T1 = ( T1 * kd_phys1) * mpo3s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo3s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo3s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys3 = delta(findtype(T3.inds(),PHYS), mpo3s.Is3);
	T3 = ( T3 * kd_phys3 ) * mpo3s.H3;
	T3 = (T3 * kd_phys3.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2-H3 to |123> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);
    PrintData(T3);

	/*
	 * Obtain new on-site tensors |1~~| by contrating tensors 1~,2~ and 3~
	 * along common indices
	 *
	 *       _________                  s1              s2 s3 
	 *  s1~~|         |~~s2            _|_              _|__|_
	 *    --| 1~ 2~ 3~|~~s3    ==>  --|   |            |      |--
	 *    --|         |--      ==>  --|1~~|++|SV_L12|++|2~3~  |--
	 *    --|_________|--           --|___|            |______|--
	 *          | |                                     | |
	 *
	 * where 1~~ and 2~3~ are now holding singular vectors wrt
	 * to SVD done along "link" between sites 1 and "rest"
	 *
	 */
	ITensor temp = ((( T1*delta(link12.first, link12.second) )*T2 )
		*delta(link23.first, link23.second) )*T3;

	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	
	ITensor SV_L12,temp2;
	Args args = Args::global();
    args.add("Maxm", auxBondDim);
	auto spec = svd(temp, T1, SV_L12, temp2, args);
	Print(spec);

	Print(T1);
	PrintData(SV_L12);
	Print(temp2);

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(temp2,SV_L12);
	temp2 = ( temp2*SV_L12 )*delta(a1,a2);

	Print(temp2);

	/*
	 * Perform SVD of new on-site tensors |2~~| and |3~| by contrating them
	 * along common index
	 *
	 *       ______               s2               s3
	 *  s2~~|      |~~s3           |                |
	 *    ++| 2~ 3~|--    ==>    |    |            |   |--
	 *      |      |--    ==>  ++|2~~ |++|SV_L23|++|3~~|--
	 *      |______|--           |____|            |___|--
	 *       | |                   | |
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link23 -----"<< std::endl;
	ITensor SV_L23;
	spec = svd(temp2, T3, SV_L23, T2, args);
	Print(spec);

	Print(T2);
	PrintData(SV_L23);
	Print(T3);

	/*
	 * We absorb SV matrices into new on-site tensors in symmetric fashion,
	 * absorbing square root of SV matrix of the link to both on-site
	 * tensors which it connects. Then we discard the excess SV values
	 * to reduce back to the auxBond dimension D
	 *
	 * ++|SV_L12|++ => ++|SV_L12^1/2|++|SV_L12^1/2|++
	 *
	 *     s1
	 *     |
	 *  --|   |                                     ==>    \ |
	 *  --|1~~|++|SV_L12^1/2|++|dR1|--link12.first  ==>  --|1n|~~s1
	 *  --|___|                                     ==>      |
	 *
	 *  where dR is a reduction tensor, which discards all the SV values
	 *  except the first D. Analogous operation is performed on the |2~~~|
	 *  on-site tensor
	 *
	 *                                        s2
     *                                        |       
	 *                                      |    |    ==>   \ |
	 *  link12.second--|dR2|++|SV_L12^1/2|++|2~~~|++  ==> --|2n|~~s2
	 *                                      |____|    ==>     |
	 *                                        ||
	 */

	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	SV_L12.apply(sqrtT);
	//T1 = ( T1*SV_L12 )*delta( a2, link12.first );
	//T2 = ( T2*SV_L12 )*delta( a1, link12.second );
	T1 = T1*delta( a1, link12.first );
	T2 = T2*delta( a2, link12.second );

	PrintData(SV_L12);
	PrintData(T1);
	Print(T2);

	std::cout <<"----- (NOT)Reducing dimension of link23 -----"<< std::endl;
	SV_L23.apply(sqrtT);
	auto a3 = commonIndex(T2,SV_L23);
	auto a4 = commonIndex(T3,SV_L23);
	T2 = ( T2*SV_L23 )*delta( a4, link23.first );
	T3 = ( T3*SV_L23 )*delta( a3, link23.second );

	PrintData(SV_L23);
	PrintData(T2);
	PrintData(T3);
}

void applyH_123_v6(MPO_3site const& mpo3s, 
	ITensor & T1, ITensor & T2, ITensor & T3, 
	std::pair<Index, Index> const& link12, 
	std::pair<Index, Index> const& link23) {

	std::cout <<">>>>> applyH_123_v5 called <<<<<"<< std::endl;
	std::cout << mpo3s; 
	PrintData(mpo3s.H1);
    PrintData(mpo3s.H2);
    PrintData(mpo3s.H3);

    int auxBondDim = link12.first.m();
	std::cout <<"link12 "<< link12.first <<" "<< link12.second << std::endl;
	std::cout <<"link23 "<< link23.first <<" "<< link23.second << std::endl;

	// Take the square-root of SV's
	double pw;
	auto sqrtT = [](double r) { return sqrt(r); };
	auto powT = [&pw](double r) { return std::pow(r,pw); };

	/*
	 * Applying 3-site MPO leads to a new tensor network of the form
	 * 
	 *    \ |    __               s1    s2    s3
	 *   --|1|~~|H1|~~s1          |_    |_    |_
	 *    \ |     |       ==   --|  |  |  |  |  |--
	 *   --|2|~~|H2|~~s2  ==   --|1~|==|2~|==|3~|--  
	 *    \ |     |       ==   --|__|  |__|  |__|--
	 *   --|3|~~|H3|~~s3                ||
	 *      |                       
	 *
	 * Indices s1,s2,s3 are relabeled back to physical indices of 
	 * original sites 1,2 and 3 after applying MPO.
	 * Now auxiliary index linking sites 1,2 & 3 have dimension increased 
	 * from D to D*auxDim of applied 3-site MPO. To obtain the network 
	 * of the original size, we have to reduce dimensions of these links 
	 * back to D by performing SVDs along links 12 and 23.
	 *
	 */

	std::cout <<"----- Initial |123> -----"<< std::endl;
	Print(T1);
	Print(T2);
	Print(T3);

	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys1 = delta(findtype(T1.inds(),PHYS), mpo3s.Is1);
	T1 = ( T1 * kd_phys1) * mpo3s.H1;
	T1 = (T1 * kd_phys1.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s^2
	ITensor kd_phys2 = delta(findtype(T2.inds(),PHYS), mpo3s.Is2);
	T2 = ( T2 * kd_phys2 ) * mpo3s.H2;
	T2 = (T2 * kd_phys2.prime()).prime(PHYS,-1);
	// D^4 x Ds x auxD_mpo3s
	ITensor kd_phys3 = delta(findtype(T3.inds(),PHYS), mpo3s.Is3);
	T3 = ( T3 * kd_phys3 ) * mpo3s.H3;
	T3 = (T3 * kd_phys3.prime()).prime(PHYS,-1);

	std::cout <<"----- Appyling H1-H2-H3 to |123> -----"<< std::endl;
	PrintData(T1);
    PrintData(T2);
    PrintData(T3);

	/*
	 * Obtain new on-site tensors |1~~| by contrating tensors 1~,2~ and 3~
	 * along common indices
	 *
	 *       _________                  s1              s2 s3 
	 *  s1~~|         |~~s2            _|_              _|__|_
	 *    --| 1~ 2~ 3~|~~s3    ==>  --|   |            |      |--
	 *    --|         |--      ==>  --|1~~|++|SV_L12|++|2~3~  |--
	 *    --|_________|--           --|___|            |______|--
	 *          | |                                     | |
	 *
	 * where 1~~ and 2~3~ are now holding singular vectors wrt
	 * to SVD done along "link" between sites 1 and "rest"
	 *
	 */
	ITensor temp = ((( T1*delta(link12.first, link12.second) )*T2 )
		*delta(link23.first, link23.second) )*T3;

	std::cout <<"----- Perform SVD along link12 -----"<< std::endl;
	
	ITensor SV_L12,temp2;
	Args args = Args::global();
    args.add("Maxm", auxBondDim);
	auto spec = svd(temp, T1, SV_L12, temp2, args);
	Print(spec);

	Print(T1);
	PrintData(SV_L12);
	Print(temp2);

	auto a1 = commonIndex(T1,SV_L12);
	auto a2 = commonIndex(temp2,SV_L12);
	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	pw = 2.0/3.0;
	SV_L12.apply(powT);
	PrintData(SV_L12);
	temp2 = ( temp2*SV_L12 )*delta(a1,a2);
	Print(temp2);

	/*
	 * Perform SVD of new on-site tensors |2~~| and |3~| by contrating them
	 * along common index
	 *
	 *       ______               s2               s3
	 *  s2~~|      |~~s3           |                |
	 *    ++| 2~ 3~|--    ==>    |    |            |   |--
	 *      |      |--    ==>  ++|2~~ |++|SV_L23|++|3~~|--
	 *      |______|--           |____|            |___|--
	 *       | |                   | |
	 *
	 * where 1~~ and 2~~ are now holding singular vectors wrt
	 * to SVD done along link between sites 1 and 2
	 *
	 */
	std::cout <<"----- Perform SVD along link23 -----"<< std::endl;
	ITensor SV_L23;
	spec = svd(temp2, T3, SV_L23, T2, args);
	Print(spec);

	Print(T2);
	PrintData(SV_L23);
	Print(T3);
	/*
	 * We absorb SV matrices into new on-site tensors in symmetric fashion,
	 * absorbing square root of SV matrix of the link to both on-site
	 * tensors which it connects. Then we discard the excess SV values
	 * to reduce back to the auxBond dimension D
	 *
	 * ++|SV_L12|++ => ++|SV_L12^1/2|++|SV_L12^1/2|++
	 *
	 *     s1
	 *     |
	 *  --|   |                                     ==>    \ |
	 *  --|1~~|++|SV_L12^1/2|++|dR1|--link12.first  ==>  --|1n|~~s1
	 *  --|___|                                     ==>      |
	 *
	 *  where dR is a reduction tensor, which discards all the SV values
	 *  except the first D. Analogous operation is performed on the |2~~~|
	 *  on-site tensor
	 *
	 *                                        s2
     *                                        |       
	 *                                      |    |    ==>   \ |
	 *  link12.second--|dR2|++|SV_L12^1/2|++|2~~~|++  ==> --|2n|~~s2
	 *                                      |____|    ==>     |
	 *                                        ||
	 */
	std::cout <<"----- (NOT)Reducing dimension of link12 -----"<< std::endl;
	SV_L12.apply(sqrtT);
	PrintData(SV_L12);
	T1 = ( T1 * SV_L12 )*delta( a2, link12.first );
	T2 = T2*delta( a2, link12.second );

	PrintData(T1);
	Print(T2);

	std::cout <<"----- (NOT)Reducing dimension of link23 -----"<< std::endl;
	SV_L23.apply(sqrtT);
	auto a3 = commonIndex(T2,SV_L23);
	auto a4 = commonIndex(T3,SV_L23);
	T2 = ( T2*SV_L23 )*delta( a4, link23.first );
	T3 = ( T3*SV_L23 )*delta( a3, link23.second );

	PrintData(SV_L23);
	PrintData(T2);
	PrintData(T3);
}

std::ostream& 
operator<<(std::ostream& s, MPO_2site const& mpo2s) {
	s <<"----- BEGIN MPO_2site "<< std::string(50,'-') << std::endl;
	s << mpo2s.Is1 <<" "<< mpo2s.Is2 << std::endl;
	s <<"H1 "<< mpo2s.H1 << std::endl;
	s <<"H2 "<< mpo2s.H2;
	s <<"----- END MPO_2site "<< std::string(52,'-') << std::endl;
	return s; 
}

std::ostream& 
operator<<(std::ostream& s, MPO_3site const& mpo3s) {
	s <<"----- BEGIN MPO_3site "<< std::string(50,'-') << std::endl;
	s << mpo3s.Is1 <<" "<< mpo3s.Is2 <<" "<< mpo3s.Is3 << std::endl;
	s <<"H1 "<< mpo3s.H1 << std::endl;
	s <<"H2 "<< mpo3s.H2 << std::endl;
	s <<"H3 "<< mpo3s.H3;
	s <<"----- END MPO_3site "<< std::string(52,'-') << std::endl;
	return s; 
} 