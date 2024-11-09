#ifndef slic3r_FillSLMisland_hpp_
#define slic3r_FillSLMisland_hpp_

#include "../libslic3r.h"

#include "Fill.hpp"

namespace Slic3r {

class FillSLMisland : public Fill
{
public:
   
    virtual Fill* clone() const { return new FillSLMisland(*this); };
    virtual ~FillSLMisland() {}
    virtual bool can_solid() const { return true; };

protected:
	virtual void _fill_surface_single(
	    unsigned int                   thickness_layers,
	    const direction_t              &direction, 
	   ExPolygon                       &expolygon , 
	    Polylines*                     polylines_out);
    
	void _fill_single_direction(ExPolygon expolygon, const direction_t &direction,
	    coord_t x_shift, Polylines* out);




		
	};

/*class FillAlignedRectilinear : public FillSLMisland
{
public:
    virtual Fill* clone() const { return new FillAlignedRectilinear(*this); };
    virtual ~FillAlignedRectilinear() {};
    virtual bool can_solid() const { return false; };

protected:
	// Keep the angle constant in all layers.
    virtual float _layer_angle(size_t idx) const { return 0.f; };
};*/

/*class DMLSFillGrid : public FillSLMisland
{
public:
    virtual Fill* clone() const { return new DMLSFillGrid(*this); };
    virtual ~DMLSFillGrid() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
 unsigned int                    thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      Gridpolys_out);

	
	//coordf_t island_HInt=scale_(6);
	//coordf_t island_LInt=scale_(6);
	//coordf_t HatchDistanInt=scale_(0.1);
	//Polygon Conto;
	//Polygons Holes;
  
	//void BoundRect(ClipperLib::Path Cont);
    //ClipperLib::Paths solution;
	//Polygons rectangularGrids;
	//ClipperLib::IntPoint Rmin;
    //ClipperLib::IntPoint Rmax;
	void RectGrids();
	void BoundRect();
	ClipperLib::Paths solution;
	ClipperLib::Paths rectangularGrids;
	ClipperLib::IntPoint Rmin;
    ClipperLib::IntPoint Rmax;
	
	
	coord_t island_H=6;
	coord_t island_L=6;
	coord_t HatchDistan=0.1;
	Polygon Conto;
	Polygons Holes;	
		
			
		};*/

/*class DMLSFillTriangles : public FillSLMisland
{
public:
    virtual Fill* clone() const { return new DMLSFillTriangles(*this); };
    virtual ~DMLSFillTriangles() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
};

class DMLSFillStars : public FillSLMisland
{
public:
    virtual Fill* clone() const { return new DMLSFillStars(*this); };
    virtual ~DMLSFillStars() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
};

class DMLSFillCubic : public FillSLMisland
{
public:
    virtual Fill* clone() const { return new DMLSFillCubic(*this); };
    virtual ~DMLSFillCubic() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
};*/

}; // namespace Slic3r

#endif // slic3r_FillDMLSisland_hpp_
