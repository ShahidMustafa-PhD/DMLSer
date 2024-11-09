#include "Surface.hpp"
#include "ClipperUtils.hpp"
namespace Slic3r {

Surface::operator Polygons() const
{
    return this->expolygon;
}

double
Surface::area() const
{
    return this->expolygon.area();
}

bool
Surface::is_solid() const
{
    return this->surface_type == stTop
        || this->surface_type == stBottom
        || this->surface_type == stBottomBridge
        || this->surface_type == stInternalSolid
        || this->surface_type == stInternalBridge
		|| this->surface_type == stInternal;
}

bool
Surface::is_external() const
{
    return this->surface_type == stTop
        || this->surface_type == stBottom
        || this->surface_type == stBottomBridge;
}

bool
Surface::is_internal() const
{
    return this->surface_type == stInternal  // means solid inside.
        //|| this->surface_type == stInternalBridge
        || this->surface_type == stInternalSolid 
        || this->surface_type == stInternalVoid;
}

bool
Surface::is_bottom() const
{
    return this->surface_type == stBottom   //  overhang
        || this->surface_type == stBottomBridge;  //  top of previous layer overhangs.
}
bool
Surface::is_bridge() const
{
    return this->surface_type == stBottom   //  overhang
        || this->surface_type == stBottomBridge;  //  top of previous layer overhangs.
}

bool
Surface::is_overhang() const
{
    return this->surface_type == stBottom;   //  overhang
       // || this->surface_type == stBottomBridge;  //  top of previous layer overhangs.
}

bool
Surface::is_over_overhang() const
{
    return this->surface_type == stBottomBridge;  //  overhang
       //|| this->surface_type ==   //  top of previous layer overhangs.
}

bool
Surface::is_core() const
{
    return this->surface_type == stInternal;  // means solid inside.
        //|| this->surface_type == stInternalBridge
       // || this->surface_type == stInternalSolid 
       // || this->surface_type == stInternalVoid;
}
bool
Surface::is_top() const
{
    return this->surface_type == stTop;

}
bool
Surface::is_support() const
{
    return this->surface_type == stSupport;  // //  ov the anchors..
        //|| this->surface_type == stInternalBridge;
}
bool 
Surface::contains(const Point &point) const
{  //const Point p=point;
	return this->expolygon.contains(point);
}

ExPolygons
Surface::RectGrids(float island_L,float island_H) const
{   
    BoundingBox R=this->expolygon.bounding_box();
    Surfaces  Grid_surfaces;
	//SurfaceCollection Grid_surfaces;//::append(const ExPolygons &src, SurfaceType surfaceType)
	Point size = R.size();
       // this->size = Point3(scale_(size.x), scale_(size.y), scale_(size.z));
	int correctL= std::ceil(size.x/scale_(island_L));
	int correctH= std::ceil(size.y/scale_(island_H));
	if (correctL==0)   correctL=1;
	if (correctH==0)   correctH=1;
	coord_t island_LInt=size.x/correctL;
	coord_t island_HInt=size.y/correctH;
	//coord_t island_HInt=scale_(this->config.island_H.value);
   // coord_t island_LInt=scale_(this->config.island_L.value);
	
	
	
	Polygons rectangularGrids;
	
	for(coord_t ycc=R.min.y; ycc < R.max.y; ycc+=island_HInt)
    {  ycc+=20;//scale_(this->config.SLM_overlap.value);//HatchDistanInt;
        		
		for(coord_t xcc=R.min.x; xcc < R.max.x; xcc+=island_LInt)
        {  xcc+=20;//scale_(this->config.SLM_overlap.value);//HatchDistanInt;
	        Points gridp;
			gridp.push_back(Point(xcc,ycc));
	        gridp.push_back(Point(xcc,ycc+island_HInt));
	        gridp.push_back(Point(xcc+island_LInt,ycc+island_HInt));
            gridp.push_back(Point(xcc+island_LInt,ycc));
		    rectangularGrids.push_back(Polygon(gridp));
 
			
        }
    }
	    
		//ExPolygons pp= intersection_ex(rectangularGrids, Polygons(this->expolygon));
	
		//intersection_ex(const Slic3r::Polygons &subject, const Slic3r::Polygons &clip, bool safety_offset_ = false)
//{if(this->expolygon.holes.empty())
	    //Grid_surfaces.append(pp,this->surface_type);
	   //else 
	 //  Grid_surfaces.append(diff_ex(pp,this->expolygon.holes),this->surface_type);
	

return     intersection_ex(rectangularGrids, Polygons(this->expolygon));
  
  }
}
