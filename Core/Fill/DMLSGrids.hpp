#ifndef DMLSGrids_H
#define DMLSGrids_H
#include "DMLSPrint.hpp"
#include "ClipperUtils.hpp"
#include "ExtrusionEntity.hpp"
#include "Fill/Fill.hpp"
#include "Geometry.hpp"
#include "Surface.hpp"
#include <iostream>
#include <complex>
#include <cstdio>
#include <vector>
#include "clipper.hpp"
#include <iostream>
#include <fstream>
#include "../libslic3r.h"
#include "Fill.hpp"

namespace Slic3r {
//using namespace std;
//using namespace ClipperLib;
class DMLSGrids: public Fill
{
public:
public:
    virtual Fill* clone() const { return new DMLSGrids(*this); };
    virtual ~DMLSGrids() {}
	
	protected:
	virtual void _MakeGrids(
	
	const direction_t               &direction, 
	    ExPolygon                       &expolygon, 
	    ExPolygon*                      Grid_out);
    
	//void _fill_single_direction(ExPolygon expolygon, const direction_t &direction,
	// coord_t x_shift, Polylines* out);
    
	
	//Slic3r::ExPolygons islandFills(float gridx,float gridy,float HatchDistan,const ExPolygon &_expolygon);
    //std::ofstream clipdatafile;

};
}; //  name space..

#endif // DMLSGrids_H
