#ifndef __CLS_BASIC_H_
#define __CLS_BASIC_H_


#include <iostream>
#include <vector>
#include <map>
#include "json.hpp"
#include "itensor/all.h"
#include "ctm-cluster.h"

namespace itensor {

/* 
 *   0 1 2
 * 0 A A A
 * 1 A A A
 * 2 A A A
 *
 */
struct Cluster_1x1_A : Cluster {

	Cluster_1x1_A() : Cluster(1,1) {}

	std::string virtual vertexToId(Vertex const& v) const override { 
		auto elemV = Vertex(0,0);
		return vToId.at(elemV); 
    }

};


/* 
 *   0 1 2         0 1 
 * 0 A B A  -->  0 A B and shift rule 
 * 1 B A B
 * 2 A B A
 *
 */
struct Cluster_2x2_ABBA : Cluster {

	Cluster_2x2_ABBA() : Cluster(2,1) {}

	std::string virtual vertexToId(Vertex const& v) const override { 
		auto elemV = Vertex((v.r[0]+v.r[1]) % 2, 0);
		return vToId.at(elemV);
    }

};

/* 
 *   0 1 2 3
 * 0 A B A B
 * 1 C D C D
 * 2 A B A A
 *
 */
struct Cluster_2x2_ABCD : Cluster {

	Cluster_2x2_ABCD() : Cluster(2,2) {}

	Cluster_2x2_ABCD(int ad, int pd) : Cluster(2,2,ad,pd) {
    	siteIds = { "A", "B", "C", "D" };
    	SI = { {"A",0}, {"B",1}, {"C",2}, {"D",3} };

    	cToS  = {
	        {std::make_pair(0,0),"A"},
	        {std::make_pair(1,0),"B"},
	        {std::make_pair(0,1),"C"},
	        {std::make_pair(1,1),"D"}
	    };
	    vToId = { {{0,0},"A"}, {{1,0},"B"}, {{0,1},"C"}, {{1,1},"D"} };

	    auto aIA = Index(TAG_I_AUX, auxBondDim, AUXLINK);
	    auto aIB = Index(TAG_I_AUX, auxBondDim, AUXLINK);
	    auto aIC = Index(TAG_I_AUX, auxBondDim, AUXLINK);
	    auto aID = Index(TAG_I_AUX, auxBondDim, AUXLINK);
	    auto pIA = Index(TAG_I_PHYS, physDim, PHYS);
	    auto pIB = Index(TAG_I_PHYS, physDim, PHYS);
	    auto pIC = Index(TAG_I_PHYS, physDim, PHYS);
	    auto pID = Index(TAG_I_PHYS, physDim, PHYS);

	    aux  = {aIA, aIB, aIC, aID};
    	phys = {pIA, pIB, pIC, pID};
    	maux  = {{"A",aIA},{"B",aIB},{"C",aIC},{"D",aID}};
    	mphys = {{"A",pIA},{"B",pIB},{"C",pIC},{"D",pID}};

	    auto A = ITensor(aIA, prime(aIA,1), prime(aIA,2), prime(aIA,3), pIA);
	    auto B = ITensor(aIB, prime(aIB,1), prime(aIB,2), prime(aIB,3), pIB);
	    auto C = ITensor(aIC, prime(aIC,1), prime(aIC,2), prime(aIC,3), pIC);
	    auto D = ITensor(aID, prime(aID,1), prime(aID,2), prime(aID,3), pID);

    	sites = {{"A", A}, {"B", B}, {"C",C}, {"D",D}};

	    // Define siteToWeights
	    siteToWeights["A"] = {
	        {{"A","B"},{2,0},"L1"},
	        {{"A","B"},{0,2},"L2"},
	        {{"A","C"},{1,3},"L3"},
	        {{"A","C"},{3,1},"L4"}
	    };
	    siteToWeights["B"] = {
	        {{"B","A"},{2,0},"L2"},
	        {{"B","A"},{0,2},"L1"},
	        {{"B","D"},{1,3},"L5"},
	        {{"B","D"},{3,1},"L6"}
	    };
	    siteToWeights["C"] = {
	        {{"C","D"},{2,0},"L7"},
	        {{"C","D"},{0,2},"L8"},
	        {{"C","A"},{1,3},"L4"},
	        {{"C","A"},{3,1},"L3"}
	    };
	    siteToWeights["D"] = {
	        {{"D","B"},{3,1},"L5"},
	        {{"D","B"},{1,3},"L6"},
	        {{"D","C"},{2,0},"L8"},
	        {{"D","C"},{0,2},"L7"}
	    };
	}

	Cluster_2x2_ABCD(std::string init_type, int ad, int pd) 
		: Cluster_2x2_ABCD(ad,pd) {
    	
    	if (init_type == "RND_AB") {
            init_RANDOM_BIPARTITE();
        } else if(init_type == "RANDOM") {
            init_RANDOM();
        } else if (init_type == "AFM") {
            init_AFM();
        } else if (init_type == "ALIGNX") {
            init_ALIGNX();
        } else if (init_type == "ZPRST") {
            init_ALIGNZ();
        } else if (init_type == "VBS") {
            init_VBS();
        } else {
            std::cout <<"Unsupported cluster initialization: "<< init_type << std::endl;
        }
	}

	std::string virtual vertexToId(Vertex const& v) const override { 
		auto elemV = Vertex(
			(v.r[0] + std::abs(v.r[0])*2)% 2, 
			(v.r[1] + std::abs(v.r[1])*2)% 2);
		return vToId.at(elemV);
    }

    void init_RANDOM() {
		std::cout <<"Initializing by RANDOM TENSORS"<< std::endl;

		auto shift05 = [](Real r){ return r-0.5; };
		for (auto & t : sites) { 
			randomize( t.second );
			t.second.apply(shift05);
		}
    }

    void init_RANDOM_BIPARTITE() {
    	std::cout <<"Initializing by RANDOM TENSORS A,B,C=B,D=A"<< std::endl;

        auto aIA = maux.at("A");
        auto aIB = maux.at("B");
        auto aIC = maux.at("C");
        auto aID = maux.at("D");

        auto pIA = mphys.at("A");
        auto pIB = mphys.at("B");
        auto pIC = mphys.at("C");
        auto pID = mphys.at("D");

        randomize(sites.at("A"));
        randomize(sites.at("B"));

        auto shift05 = [](double r){ return r-0.5; };
        sites.at("A").apply(shift05);
        sites.at("B").apply(shift05);

        sites.at("C") = sites.at("B") * delta(pIB, pIC);
        sites.at("D") = sites.at("A") * delta(pIA, pID);
        for (int i=0; i<=3; ++i) {
            sites.at("C") = sites.at("C") * delta(prime(aIB,i), prime(aIC,i));
            sites.at("D") = sites.at("D") * delta(prime(aIA,i), prime(aID,i));
        }
    }

    void init_AFM() {
        std::cout <<"Initializing by AFM order A=down, B=up"<< std::endl;

        auto aIA = maux.at("A");
        auto aIB = maux.at("B");
        auto aIC = maux.at("C");
        auto aID = maux.at("D");

        auto pIA = mphys.at("A");
        auto pIB = mphys.at("B");
        auto pIC = mphys.at("C");
        auto pID = mphys.at("D");

        // Spin DOWN on site A, spin   UP on site B
        // Spin UP   on site C, spin DOWN on site D
        sites.at("A").set(aIA(1), prime(aIA,1)(1), prime(aIA,2)(1), prime(aIA,3)(1),
            pIA(2), 1.0);
        sites.at("B").set(aIB(1), prime(aIB,1)(1), prime(aIB,2)(1), prime(aIB,3)(1),
            pIB(1), 1.0);
        sites.at("C").set(aIC(1), prime(aIC,1)(1), prime(aIC,2)(1), prime(aIC,3)(1),
            pIC(1), 1.0);
        sites.at("D").set(aID(1), prime(aID,1)(1), prime(aID,2)(1), prime(aID,3)(1),
            pID(2), 1.0);
	} 

	void init_ALIGNX() {
		std::cout <<"Initializing by PRODUCT STATE along X"<< std::endl;

        auto aIA = maux.at("A");
        auto aIB = maux.at("B");
        auto aIC = maux.at("C");
        auto aID = maux.at("D");

        auto pIA = mphys.at("A");
        auto pIB = mphys.at("B");
        auto pIC = mphys.at("C");
        auto pID = mphys.at("D");

        sites.at("A").set(aIA(1), prime(aIA,1)(1), prime(aIA,2)(1), prime(aIA,3)(1),
            pIA(1), 1.0/std::sqrt(2.0));
        sites.at("A").set(aIA(1), prime(aIA,1)(1), prime(aIA,2)(1), prime(aIA,3)(1),
            pIA(2), 1.0/std::sqrt(2.0));
        sites.at("B").set(aIB(1), prime(aIB,1)(1), prime(aIB,2)(1), prime(aIB,3)(1),
            pIB(1), 1.0/std::sqrt(2.0));
        sites.at("B").set(aIB(1), prime(aIB,1)(1), prime(aIB,2)(1), prime(aIB,3)(1),
            pIB(2), 1.0/std::sqrt(2.0));
        sites.at("C").set(aIC(1), prime(aIC,1)(1), prime(aIC,2)(1), prime(aIC,3)(1),
            pIC(1), 1.0/std::sqrt(2.0));
        sites.at("C").set(aIC(1), prime(aIC,1)(1), prime(aIC,2)(1), prime(aIC,3)(1),
            pIC(2), 1.0/std::sqrt(2.0));
        sites.at("D").set(aID(1), prime(aID,1)(1), prime(aID,2)(1), prime(aID,3)(1),
            pID(1), 1.0/std::sqrt(2.0));
        sites.at("D").set(aID(1), prime(aID,1)(1), prime(aID,2)(1), prime(aID,3)(1),
            pID(2), 1.0/std::sqrt(2.0));
	}

	void init_ALIGNZ() {
		std::cout <<"Initializing by PRODUCT STATE along Z +1/2"<< std::endl;

        auto aIA = maux.at("A");
        auto aIB = maux.at("B");
        auto aIC = maux.at("C");
        auto aID = maux.at("D");

        auto pIA = mphys.at("A");
        auto pIB = mphys.at("B");
        auto pIC = mphys.at("C");
        auto pID = mphys.at("D");

        // Spin UP on all sites
        sites.at("A").set(aIA(1), prime(aIA,1)(1), prime(aIA,2)(1), prime(aIA,3)(1),
            pIA(1), 1.0);
        sites.at("B").set(aIB(1), prime(aIB,1)(1), prime(aIB,2)(1), prime(aIB,3)(1),
            pIB(1), 1.0);
        sites.at("C").set(aIC(1), prime(aIC,1)(1), prime(aIC,2)(1), prime(aIC,3)(1),
            pIC(1), 1.0);
        sites.at("D").set(aID(1), prime(aID,1)(1), prime(aID,2)(1), prime(aID,3)(1),
            pID(1), 1.0);
	}

	void init_VBS() {
		std::cout <<"Initializing by VERTICAL VBS STATE"<< std::endl;

        auto aIA = maux.at("A");
        auto aIB = maux.at("B");
        auto aIC = maux.at("C");
        auto aID = maux.at("D");

        auto pIA = mphys.at("A");
        auto pIB = mphys.at("B");
        auto pIC = mphys.at("C");
        auto pID = mphys.at("D");

        // Spin UP on all sites
        sites.at("A").set(aIA(1), prime(aIA,1)(1), prime(aIA,2)(1), prime(aIA,3)(1),
            pIA(1), 1.0);
        sites.at("A").set(aIA(1), prime(aIA,1)(1), prime(aIA,2)(1), prime(aIA,3)(2),
            pIA(2), -1.0);
        sites.at("B").set(aIB(1), prime(aIB,1)(1), prime(aIB,2)(1), prime(aIB,3)(1),
            pIB(1), 1.0);
        sites.at("B").set(aIB(1), prime(aIB,1)(1), prime(aIB,2)(1), prime(aIB,3)(2),
            pIB(2), -1.0);
        sites.at("C").set(aIC(1), prime(aIC,1)(2), prime(aIC,2)(1), prime(aIC,3)(1),
            pIC(1), 1.0);
        sites.at("C").set(aIC(1), prime(aIC,1)(1), prime(aIC,2)(1), prime(aIC,3)(1),
            pIC(2), 1.0);
        sites.at("D").set(aID(1), prime(aID,1)(2), prime(aID,2)(1), prime(aID,3)(1),
            pID(1), 1.0);
        sites.at("D").set(aID(1), prime(aID,1)(1), prime(aID,2)(1), prime(aID,3)(1),
            pID(2), 1.0);
	}

};

} //

#endif