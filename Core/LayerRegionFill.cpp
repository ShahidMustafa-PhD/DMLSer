#include "Layer.hpp"
#include "ClipperUtils.hpp"
#include "Fill/Fill.hpp"
#include "Geometry.hpp"
#include "Print.hpp"
#include "PrintConfig.hpp"
#include "Surface.hpp"

namespace Slic3r {

/// Struct for the main attributes of a Surface
/// Used for comparing properties
struct SurfaceGroupAttrib
{
    SurfaceGroupAttrib() : is_solid(false), fw(0.f), pattern(-1) {}
    /// True iff all all three attributes are the same
    bool operator==(const SurfaceGroupAttrib &other) const
        { return is_solid == other.is_solid && fw == other.fw && pattern == other.pattern; }
    bool    is_solid;   ///< if a solid infill should be used
    float   fw;         ///< flow Width
    int     pattern;    ///< pattern is of type InfillPattern, -1 for an unset pattern.
};

/// The LayerRegion at this point of time may contain
/// surfaces of various types (internal/bridge/top/bottom/solid).
/// The infills are generated on the groups of surfaces with a compatible type.
/// Fills an array of ExtrusionPathCollection objects containing the infills generated now
/// and the thin fills generated by generate_perimeters().
void
LayerRegion::make_fill()
{
   // this->fills.clear();
   //std::cout << "filling\n" << std::endl ;
   //const double fill_density          = this->region()->config.fill_density;
   // const Flow   infill_flow           = this->flow(frInfill);
   // const Flow   solid_infill_flow     = this->flow(frSolidInfill);
   // const Flow   top_solid_infill_flow = this->flow(frTopSolidInfill);
    //const coord_t perimeter_spacing    = this->flow(frPerimeter).scaled_spacing();

    SurfaceCollection surfaces;
	this->fill_surfaces.set(this->slices);//  first save a copy
	//  break into grids.
	this->slices.RectGrids(this->region()->config.island_L.value,this->region()->config.island_H.value);

    // merge adjacent surfaces
    // in case of bridge surfaces, the ones with defined angle will be attached to the ones
    // without any angle (shouldn't this logic be moved to process_external_surfaces()?)
   /* {
        Polygons polygons_bridged;
		
        polygons_bridged.reserve(this->fill_surfaces.surfaces.size());
        for (Surfaces::const_iterator it = this->fill_surfaces.surfaces.begin(); it != this->fill_surfaces.surfaces.end(); ++it)
            if (it->is_bridge() && it->bridge_angle >= 0)
                append_to(polygons_bridged, (Polygons)*it);
        
        // group surfaces by distinct properties (equal surface_type, thickness, thickness_layers, bridge_angle)
        // group is of type SurfaceCollection
        // FIXME: Use some smart heuristics to merge similar surfaces to eliminate tiny regions.
        std::vector<SurfacesConstPtr> groups;
        this->fill_surfaces.group(&groups);
        
        // merge compatible solid groups (we can generate continuous infill for them)
        {
            // cache flow widths and patterns used for all solid groups
            // (we'll use them for comparing compatible groups)
            std::vector<SurfaceGroupAttrib> group_attrib(groups.size());
            for (size_t i = 0; i < groups.size(); ++i) {
                const Surface &surface = *groups[i].front();
                // we can only merge solid non-bridge surfaces, so discard
                // non-solid or bridge surfaces
                if (!surface.is_solid() || surface.is_bridge()) continue;
                
                group_attrib[i].is_solid = true;
                group_attrib[i].fw = (surface.surface_type == stTop) ? top_solid_infill_flow.width : solid_infill_flow.width;
                group_attrib[i].pattern = surface.surface_type == stTop ? this->region()->config.top_infill_pattern.value
                    : surface.is_bottom() ? this->region()->config.bottom_infill_pattern.value
                    : ipRectilinear;
            }
            // Loop through solid groups, find compatible groups and append them to this one.
            for (size_t i = 0; i < groups.size(); ++i) {
                if (!group_attrib[i].is_solid)
                    continue;
                for (size_t j = i + 1; j < groups.size();) {
                    if (group_attrib[i] == group_attrib[j]) {
                        // groups are compatible, merge them
                        append_to(groups[i], groups[j]);
                        groups.erase(groups.begin() + j);
                        group_attrib.erase(group_attrib.begin() + j);
                    } else {
                        ++j;
                    }
                }
            }
        }
        
        // Give priority to oriented bridges. Process the bridges in the first round, the rest of the surfaces in the 2nd round.
        for (size_t round = 0; round < 2; ++ round) {
            for (std::vector<SurfacesConstPtr>::const_iterator it_group = groups.begin(); it_group != groups.end(); ++ it_group) {
                const SurfacesConstPtr &group = *it_group;
                const bool is_oriented_bridge = group.front()->is_bridge() && group.front()->bridge_angle >= 0;
                if (is_oriented_bridge != (round == 0))
                    continue;
                
                // Make a union of polygons defining the infiill regions of a group, use a safety offset.
                Polygons union_p = union_(to_polygons(group), true);
                
                // Subtract surfaces having a defined bridge_angle from any other, use a safety offset.
                if (!is_oriented_bridge && !polygons_bridged.empty())
                    union_p = diff(union_p, polygons_bridged, true);
                
                // subtract any other surface already processed
                //FIXME Vojtech: Because the bridge surfaces came first, they are subtracted twice!
                surfaces.append(
                    diff_ex(union_p, to_polygons(surfaces), true),
                    *group.front()  // template
                );
            }
        }
    }*/
    
    // we need to detect any narrow surfaces that might collapse
    // when adding spacing below
    // such narrow surfaces are often generated in sloping walls
    // by bridge_over_infill() and combine_infill() as a result of the
    // subtraction of the combinable area from the layer infill area,
    // which leaves small areas near the perimeters
    // we are going to grow such regions by overlapping them with the void (if any)
    // TODO: detect and investigate whether there could be narrow regions without
    // any void neighbors
   /* {
        //coord_t distance_between_surfaces = std::max(
           // std::max(infill_flow.scaled_spacing(), solid_infill_flow.scaled_spacing()),
           // top_solid_infill_flow.scaled_spacing()
      //  );
        coord_t distance_between_surfaces=scale_(0.2);  //  force to 80 micron.
        Polygons surfaces_polygons = (Polygons)surfaces;
        Polygons collapsed = diff(
            surfaces_polygons,
            offset2(surfaces_polygons, -distance_between_surfaces/2, +distance_between_surfaces/2),
            true
        );
            
        Polygons to_subtract;
        surfaces.filter_by_type(stInternalVoid, &to_subtract);
                
        append_to(to_subtract, collapsed);
        surfaces.append(
            intersection_ex(
                offset(collapsed, distance_between_surfaces),
                to_subtract,
                true
            ),
            stInternalSolid
        );
    }*/


 //   here   surfaces are ready  for  fill 
 //  our fill_surfaces are already islands.. 
     
    for (Surfaces::const_iterator surface_it = this->slices.surfaces.begin();
        surface_it !=this->slices.surfaces.end(); ++surface_it) 
		{
        
        const Surface &surface = *surface_it;
        if (surface.surface_type == stInternalVoid)
            continue;
        
        InfillPattern core_fill_pattern  = this->region()->config.fill_pattern.value;
        
		double density = 100;//fill_density;
      
        const bool is_bridge = this->layer()->id() > 0 && surface.is_bridge();
        
       /* if (surface.is_solid()) {
            density = 100.;
            core_fill_pattern  = (surface.surface_type == stTop) ? this->region()->config.top_infill_pattern.value
                : (surface.is_bottom() && !is_bridge)      ? this->region()->config.bottom_infill_pattern.value
                : ipRectilinear;
        } else if (density <= 0)
            continue;*/
         core_fill_pattern  =  this->region()->config.top_infill_pattern.value;
       // boost::nowide::cout << "pattern: " << core_fill_pattern << std::endl;
		// get filler object
        #if SLIC3R_CPPVER >= 11
            std::unique_ptr<Fill> f = std::unique_ptr<Fill>(Fill::new_from_type(core_fill_pattern ));
        #else
            std::auto_ptr<Fill> f = std::auto_ptr<Fill>(Fill::new_from_type(core_fill_pattern ));
        #endif
        
        // switch to rectilinear if this pattern doesn't support solid infill
        /*if (density > 99 && !f->can_solid())
            #if SLIC3R_CPPVER >= 11
                f = std::unique_ptr<Fill>(Fill::new_from_type(ipRectilinear));
            #else
                f = std::auto_ptr<Fill>(Fill::new_from_type(ipRectilinear));
            #endif*/
        
        f->bounding_box = this->layer()->object()->bounding_box();
         //f->bounding_box= surface.expolygon.bounding_box();
        // calculate the actual flow we'll be using for this infill
        coordf_t h = (surface.thickness == -1) ? this->layer()->height : surface.thickness;
      /*  Flow flow = this->region()->flow(
            role,
            h,
            is_bridge || f->use_bridge_flow(),  // bridge flow?
            this->layer()->id() == 0,           // first layer?
            -1,                                 // auto width
            *this->layer()->object()
        );*/
        
        // calculate flow spacing for infill pattern generation
        bool using_internal_flow = false;
        /*if (!surface.is_solid() && !is_bridge) {
            // it's internal infill, so we can calculate a generic flow spacing
            // for all layers, for avoiding the ugly effect of
            // misaligned infill on first layer because of different extrusion width and
            // layer height
            Flow internal_flow = this->region()->flow(
                frInfill,
                this->layer()->object()->config.layer_height.value,  // TODO: handle infill_every_layers?
                false,  // no bridge
                false,  // no first layer
                -1,     // auto width
                *this->layer()->object()
            );
            //f->min_spacing =  this->config;//internal_flow.spacing();
            f->min_spacing = this->region()->config.Hatch_Spacing.value;
			//using_internal_flow = true;
        } else
			{
			 f->min_spacing = this->region()->config.Hatch_Spacing.value;
           // f->min_spacing = flow.spacing();
		   //  modify it according to configuration.
             }*/
	
        f->min_spacing = this->region()->config.Hatch_Spacing.value;
       // f->endpoints_overlap = this->region()->config.get_abs_value("infill_overlap",scale_(f->min_spacing)/2);

        f->layer_id = this->layer()->id();
        f->z        = this->layer()->print_z;
        f->angle    = Geometry::deg2rad(this->region()->config.fill_angle.value);
        
        // Maximum length of the perimeter segment linking two infill lines.
       /* f->link_max_length = (!is_bridge && density > 80)
            ? scale_(3 * f->min_spacing)
            : 0;*/
        
        // Used by the concentric infill pattern to clip the loops to create extrusion paths.
       // f->loop_clipping = scale_(flow.nozzle_diameter) * LOOP_CLIPPING_LENGTH_OVER_NOZZLE_DIAMETER;
        
        // apply half spacing using this flow's own spacing and generate infill
        f->density =1;// density/100;
        //f->dont_adjust = false;
        /*
        std::cout << surface.expolygon.dump_perl() << std::endl
            << " layer_id: " << f->layer_id << " z: " << f->z
            << " angle: " << f->angle << " min-spacing: " << f->min_spacing
            << " endpoints_overlap: " << f->endpoints_overlap << std::endl << std::endl;
        */
		//  This makes fills...
	    Lines cleanlines;
        Polylines polylines = f->fill_surface(surface);
        if (polylines.empty())
            continue;
		//
		   //  std::cout << "Polysize\n" << polylines.size()<<std::endl ;
			for (Polylines::const_iterator pl=polylines.begin(); pl !=polylines.end(); ++pl) 
	{  cleanlines=pl->scan_lines() ; // extracts lines from polyline..
				
			
				
				//Lines plines=intersection_ln(cleanlines,Polygons(surface));
		append_to(scan_vectors,intersection_ln(cleanlines,Polygons(surface)));	
              //Polygons hols=ct->holes;
			  /*if(!hols.empty())
			   append_to(scan_vectors,diff_ln(plines, hols));
			    else 
	           append_to(scan_vectors,plines);*/

       
			
	}
      //
	  //  the polylines of current fill are ready  for each region
	//  append_to(fill_slm,polylines);
	   //std::cout << "fill size\n" <<fill_slm.size()<< std::endl ;
        // calculate actual flow from spacing (which might have been adjusted by the infill
        // pattern generator)
        /*if (using_internal_flow) {
            // if we used the internal flow we're not doing a solid infill
            // so we can safely ignore the slight variation that might have
            // been applied to f->spacing()
        } else {
            flow = Flow::new_from_spacing(f->spacing(), flow.nozzle_diameter, h, is_bridge || f->use_bridge_flow());
        }*/

        // Save into layer.
        //ExtrusionEntityCollection* coll = new ExtrusionEntityCollection();
        //coll->no_sort = f->no_sort();

    
        
        {   //  decide the role here ( bridge  solid or top or bottom or overhange..)
            ExtrusionRole role;
            if (surface.is_core()) {
                role = ercoreInfill;
			  // float ScanSpeed=this->region()->config.core_scan_speed.value;
				//int  LaserPower=this->region()->config.core_laser_power.value;
            } else if (surface.is_top()) {
                role = erTopSolidInfill ;
				//float ScanSpeed=this->region()->config.core_scan_speed.value;
				//int  LaserPower=this->region()->config.core_laser_power.value;
            } else if (surface.is_overhang()) {
                role =  eroverhangInfill;
				//float ScanSpeed=this->region()->config.support_laser_power.value;
				//int  LaserPower=this->region()->config.core_laser_power.value;
            } else if (surface.is_support()) {
                role = erSupportMaterial;
				//float ScanSpeed=this->region()->config.support_laser_power.value;
				//int  LaserPower=this->region()->config.core_laser_power.value;
            } else if (surface.is_over_overhang()) {
                role = erover_overhangInfill;
				//float ScanSpeed=this->region()->config.support_laser_power.value;
				//int  LaserPower=this->region()->config.core_laser_power.value;else if (surface.is_support()) {
                
				//float ScanSpeed=this->region()->config.support_laser_power.value;
				//int  LaserPower=this->region()->config.core_laser_power.value; {
            }
            
            ExtrusionPath *templ = new  ExtrusionPath(role);
			templ->exposure_vectors=scan_vectors;
            templ->SS =10;// ScanSpeed;
            templ->LaserPower =20;//LaserPower;
            //templ.height        = flow.height;
            this->fills.ExposureVectors.push_back(templ);
			this->scan_vectors.clear();
           // coll->entities.push_back(templ);//append(STDMOVE(polylines), templ);
	
        }
    }

    // add thin fill regions
    // thin_fills are of C++ Slic3r::ExtrusionEntityCollection, perl type Slic3r::ExtrusionPath::Collection
    // Unpacks the collection, creates multiple collections per path so that they will
    // be individually included in the nearest neighbor search.
    // The path type could be ExtrusionPath, ExtrusionLoop or ExtrusionEntityCollection.
    /*for (ExtrusionEntitiesPtr::const_iterator thin_fill = this->thin_fills.entities.begin(); thin_fill != this->thin_fills.entities.end(); ++ thin_fill) {
        ExtrusionEntityCollection* coll = new ExtrusionEntityCollection();
        this->fills.entities.push_back(coll);
        coll->append(**thin_fill);
    }*/
}
void
LayerRegion::MakeGrids()

{   
// makes grids after seperating  skins..
SurfaceCollection gridcollection;
//ExPolygons offset_Slices =intersection_ex(ExPolygons(this->slices.surfaces),offset(ExPolygons(this->slices.surfaces), -scale_(this->region()->config.Hatch_Spacing/2)));
//this->skin.append(intersection_ex(this->slices,offset(offset_Slices,stSkin);
   // this->fill_surfaces.surfaces;
    //expp.reserve(this->surfaces.size());
//this->skin.appendintersection_ex(this->slices,offset(layer.slices, -scale_(this->region()->config.Hatch_Spacing/2)));
    for (Surfaces::const_iterator surface = this->slices.surfaces.begin(); surface != this->slices.surfaces.end(); ++surface) 
  {  
        SurfaceType  type=surface->surface_type;
		Holes.clear();	
		Conto=surface->expolygon.contour; 
		Polygons con;
		con.push_back(Conto);
		//offset(surface->expolygon.contour,-scale_(this->region()->config.Hatch_Spacing/2));
		 Holes= surface->expolygon.holes; 
		 //offset(surface->expolygon.holes,scale_(this->region()->config.Hatch_Spacing/2));
		RectGrids();
		Polygons pp= intersection(rectangularGrids, con);
	    gridcollection.append(diff_ex(pp,Holes),type);
		pp.clear();
		}
this->fill_surfaces.set(gridcollection);
return;
}


void 
LayerRegion::BoundRect()
{

    Rmin.x=9999999000;
    Rmin.y=9999999000;
    Rmax.x=-9999999000;
    Rmax.y=-9999999000;
         
		 for (Points::const_iterator pts = Conto.points.begin(); pts !=Conto.points.end(); ++pts)
        {
            if(pts->x <  Rmin.x)
                 Rmin.x=pts->x;
            if(pts->y < Rmin.y)
                Rmin.y=pts->y;
            if(pts->x > Rmax.x)
                   Rmax.x=pts->x;
            if(pts->y > Rmax.y)
                 Rmax.y=pts->y;
        }
   //std::cout <<  Rmin.x <<"-" <<  Rmin.y<< "-" <<  Rmax.x<<"-" <<  Rmax.y<<std::endl ;
         return;
}

 void
LayerRegion::RectGrids()
{   
    BoundRect();
    rectangularGrids.clear();
    coord_t offset1=0;//scale_(rand()%10);// random rect size between 0 and 2mm..
    float correctL= std::ceil(std::abs(Rmax.x -Rmin.x)/this->region()->config.island_L.value);
	float correctH= std::ceil(std::abs(Rmax.y -Rmin.y)/this->region()->config.island_H.value);
	if (correctL==0)   correctL=1;
	if (correctH==0)   correctH=1;
	coord_t island_LInt=scale_(std::abs(Rmax.x -Rmin.x)/correctL);
	coord_t island_HInt=scale_(std::abs(Rmax.y -Rmin.y)/correctH);
	//coord_t island_HInt=scale_(this->config.island_H.value);
   // coord_t island_LInt=scale_(this->config.island_L.value);
    coord_t HatchDistanInt=scale_(this->region()->config.Hatch_Spacing.value);
	Points gridp;
	gridp.clear();
	 //std::cout << "islandL\n" <<this->region()->config.island_L.value<<std::endl ;
   // std::cout << "islandH\n" <<this->region()->config.island_H.value<<std::endl ;
	for(coord_t ycc=Rmin.y; ycc < Rmax.y; ycc+=island_HInt)
    {  ycc+=2;//scale_(this->config.SLM_overlap.value);//HatchDistanInt;
        		
		for(coord_t xcc=Rmin.x; xcc < Rmax.x; xcc+=island_LInt)
        {  xcc+=2;//scale_(this->config.SLM_overlap.value);//HatchDistanInt;
	        gridp.push_back(Point(xcc,ycc));
	        gridp.push_back(Point(xcc,ycc+island_HInt));
	        gridp.push_back(Point(xcc+island_LInt,ycc+island_HInt));
            gridp.push_back(Point(xcc+island_LInt,ycc));
	        rectangularGrids.push_back(Polygon(gridp));
            gridp.clear();
			

        }
    }

return;
  
  }

  
} // namespace Slic3r
