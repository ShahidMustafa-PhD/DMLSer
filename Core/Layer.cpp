#include "Layer.hpp"
#include "ClipperUtils.hpp"
#include "Geometry.hpp"
#include "Print.hpp"
#include <iostream>
namespace Slic3r {

/// Initialises upper_layer, lower_layer to NULL
/// Initialises slicing_errors to false
Layer::Layer(size_t id, PrintObject *object, coordf_t height, coordf_t print_z,
        coordf_t slice_z)
:   upper_layer(NULL),
    lower_layer(NULL),
    regions(),
    slicing_errors(false),
    slice_z(slice_z),
    print_z(print_z),
    height(height),
    slices(),
    _id(id),
    _object(object)
{
}

/// Removes references to self and clears regions
Layer::~Layer()
{
    // remove references to self
    if (NULL != this->upper_layer) {
        this->upper_layer->lower_layer = NULL;
    }

    if (NULL != this->lower_layer) {
        this->lower_layer->upper_layer = NULL;
    }

    this->clear_regions();
}

/// Getter for this->_id
size_t
Layer::id() const
{
    return this->_id;
}

/// Setter for this->_id
void
Layer::set_id(size_t id)
{
    this->_id = id;
}

/// Getter for this->regions.size()
size_t
Layer::region_count() const
{
    return this->regions.size();
}

/// Deletes all regions using this->delete_region()
void
Layer::clear_regions()
{
    for (int i = this->regions.size()-1; i >= 0; --i)
        this->delete_region(i);
}

/// Creates a LayerRegion from a PrintRegion and adds it to this->regions
LayerRegion*
Layer::add_region(PrintRegion* print_region)
{
    LayerRegion* region = new LayerRegion(this, print_region);
    this->regions.push_back(region);
    return region;
}

/// Deletes an individual region
void
Layer::delete_region(int idx)
{
    LayerRegionPtrs::iterator i = this->regions.begin() + idx;
    LayerRegion* item = *i;
    this->regions.erase(i);
    delete item;
}

/// Merge all regions' slices to get islands
//TODO: is this right?
void
Layer::make_slices()
{
    ExPolygons slices;
    if (this->regions.size() == 1) {
        // optimization: if we only have one region, take its slices
        slices = this->regions.front()->slices;
    } else {
        Polygons slices_p;
        FOREACH_LAYERREGION(this, layerm) {
            Polygons region_slices_p = (*layerm)->slices;
            slices_p.insert(slices_p.end(), region_slices_p.begin(), region_slices_p.end());
        }
        slices = union_ex(slices_p);
    }
    
    this->slices.expolygons.clear();
    this->slices.expolygons.reserve(slices.size());
    
    // prepare ordering points
    // While it's more computationally expensive, we use centroid()
    // instead of first_point() because it's [much more] deterministic
    // and preserves ordering across similar layers.
    Points ordering_points;
    ordering_points.reserve(slices.size());
    for (const ExPolygon &ex : slices)
        ordering_points.push_back(ex.contour.centroid());
    
    // sort slices
    std::vector<Points::size_type> order;
    Slic3r::Geometry::chained_path(ordering_points, order);
    
    // populate slices vector
    for (const Points::size_type &o : order)
        this->slices.expolygons.push_back(slices[o]);
}

/// Iterates over all of the LayerRegion and invokes LayerRegion->merge_slices()
void
Layer::merge_slices()
{
    FOREACH_LAYERREGION(this, layerm) {
        (*layerm)->merge_slices();
    }
}

/// Uses LayerRegion->slices.any_internal_contains(item)
template <class T>
bool
Layer::any_internal_region_slice_contains(const T &item) const
{
    FOREACH_LAYERREGION(this, layerm) {
        if ((*layerm)->slices.any_internal_contains(item)) return true;
    }
    return false;
}
template bool Layer::any_internal_region_slice_contains<Polyline>(const Polyline &item) const;

/// Uses LayerRegion->slices.any_bottom_contains(item)
template <class T>
bool
Layer::any_bottom_region_slice_contains(const T &item) const
{
    FOREACH_LAYERREGION(this, layerm) {
        if ((*layerm)->slices.any_bottom_contains(item)) return true;
    }
    return false;
}
template bool Layer::any_bottom_region_slice_contains<Polyline>(const Polyline &item) const;

/// -The perimeter paths and the thin fills (ExtrusionEntityCollection) are assigned to the first compatible layer region.
/// -The resulting fill surface is split back among the originating regions.
void
Layer::make_perimeters()
{
    #ifdef SLIC3R_DEBUG
    printf("Making perimeters for layer %zu\n", this->id());
    #endif
    
    // keep track of regions whose perimeters we have already generated
    std::set<size_t> done;
    
    FOREACH_LAYERREGION(this, layerm) {
        size_t region_id = layerm - this->regions.begin();
        if (done.find(region_id) != done.end()) continue;
        done.insert(region_id);
        const PrintRegionConfig &config = (*layerm)->region()->config;
        
        // find compatible regions
        LayerRegionPtrs layerms;
        layerms.push_back(*layerm);
        for (LayerRegionPtrs::const_iterator it = layerm + 1; it != this->regions.end(); ++it) {
            LayerRegion* other_layerm = *it;
            const PrintRegionConfig &other_config = other_layerm->region()->config;
            
            if (config.perimeter_extruder   == other_config.perimeter_extruder
                && config.perimeters        == other_config.perimeters
                && config.perimeter_speed   == other_config.perimeter_speed
                && config.gap_fill_speed    == other_config.gap_fill_speed
                && config.overhangs         == other_config.overhangs
                && config.serialize("perimeter_extrusion_width").compare(other_config.serialize("perimeter_extrusion_width")) == 0
                && config.thin_walls        == other_config.thin_walls
                && config.external_perimeters_first == other_config.external_perimeters_first) {
                layerms.push_back(other_layerm);
                done.insert(it - this->regions.begin());
            }
        }
        
        if (layerms.size() == 1) {  // optimization
            (*layerm)->fill_surfaces.surfaces.clear();
            (*layerm)->make_perimeters((*layerm)->slices, &(*layerm)->fill_surfaces);
        } else {
            // group slices (surfaces) according to number of extra perimeters
            std::map<unsigned short,Surfaces> slices;  // extra_perimeters => [ surface, surface... ]
            for (LayerRegionPtrs::iterator l = layerms.begin(); l != layerms.end(); ++l) {
                for (Surfaces::iterator s = (*l)->slices.surfaces.begin(); s != (*l)->slices.surfaces.end(); ++s) {
                    slices[s->extra_perimeters].push_back(*s);
                }
            }
            
            // merge the surfaces assigned to each group
            SurfaceCollection new_slices;
            for (const auto &it : slices) {
                ExPolygons expp = union_ex(it.second, true);
                for (ExPolygon &ex : expp) {
                    Surface s = it.second.front();  // clone type and extra_perimeters
                    s.expolygon = ex;
                    new_slices.surfaces.push_back(s);
                }
            }
            
            // make perimeters
            SurfaceCollection fill_surfaces;
            (*layerm)->make_perimeters(new_slices, &fill_surfaces);
            
            // assign fill_surfaces to each layer
            if (!fill_surfaces.surfaces.empty()) {
                for (LayerRegionPtrs::iterator l = layerms.begin(); l != layerms.end(); ++l) {
                    ExPolygons expp = intersection_ex(
                        (Polygons) fill_surfaces,
                        (Polygons) (*l)->slices
                    );
                    (*l)->fill_surfaces.surfaces.clear();
                    
                    for (ExPolygons::iterator ex = expp.begin(); ex != expp.end(); ++ex) {
                        Surface s = fill_surfaces.surfaces.front();  // clone type and extra_perimeters
                        s.expolygon = *ex;
                        (*l)->fill_surfaces.surfaces.push_back(s);
                    }
                }
            }
        }
    }
}

/// -Iterates over all of the LayerRegion and invokes LayerRegion->make_fill()
/// -Asserts that the fills created are not NULL
void
Layer::make_fills()
{
    #ifdef SLIC3R_DEBUG
    printf("Making fills for layer %zu\n", this->id());
    #endif
    
    FOREACH_LAYERREGION(this, it_layerm) {
        (*it_layerm)->make_fill();
        
        #ifndef NDEBUG
        for (size_t i = 0; i < (*it_layerm)->fills.entities.size(); ++i)
            assert(dynamic_cast<ExtrusionEntityCollection*>((*it_layerm)->fills.entities[i]) != NULL);
        #endif
    }
}

/// -Analyzes slices of a region (SurfaceCollection slices).
/// -Each region slice (instance of Surface) is analyzed, whether it is supported or whether it is the top surface.
/// -Initially all slices are of type S_TYPE_INTERNAL.
/// -Slices are compared against the top / bottom slices and regions and classified to the following groups:
/// -S_TYPE_TOP - Part of a region, which is not covered by any upper layer. This surface will be filled with a top solid infill.
/// -S_TYPE_BOTTOMBRIDGE - Part of a region, which is not fully supported, but it hangs in the air, or it hangs losely on a support or a raft.
/// -S_TYPE_BOTTOM - Part of a region, which is not supported by the same region, but it is supported either by another region, or by a soluble interface layer.
/// -S_TYPE_INTERNAL - Part of a region, which is supported by the same region type.
/// -If a part of a region is of S_TYPE_BOTTOM and S_TYPE_TOP, the S_TYPE_BOTTOM wins.
void
Layer::detect_surfaces_type()
{
    PrintObject &object = *this->object();
     
	float oh_angle; //  radians
	coord_t oh_permissible ;//=scale_(this->height/tan(oh_angle)); // mm
    std::cout << "Layer number: " <<this->id()<< std::endl;
	 //boost::nowide::cout << "OH Ang: " <<oh_angle<< " OH"<< oh_permissible << std::endl;
    Layer*  &upper_layer = this->upper_layer;
    Layer*  &lower_layer = this->lower_layer;
	SurfaceType surface_type_bottom;
	for (size_t region_id = 0; region_id < this->regions.size(); ++region_id) {
        LayerRegion &layerm = *this->regions[region_id];
        oh_angle=0.01745*(layerm.region()->config.support_material_angle.value+1);
	    oh_permissible =scale_(this->height/tan(oh_angle)); 
        // -collapse very narrow parts (using the safety offset in the diff is not enough)
        // -TODO: this offset2 makes this method not idempotent (see #3764), so we should
        // -move it to where we generate fill_surfaces instead and leave slices unaltered
        const float offs = 1;//layerm.flow(frExternalPerimeter).scaled_width() / 10.f;

        //-const
		//-ExPolygons layerm_slices_surfaces = layerm.slices; 
	
        ExPolygons layerm_slices_surfaces=layerm.slices;//offset_ex( ExPolygons(layerm.slices), - scale_(layerm.region()->config.Hatch_Spacing.value/2));
		layerm.skin= layerm_slices_surfaces;
		//diff_ex(ExPolygons(layerm.slices),layerm_slices_surfaces);
        //layerm_slices_surfaces=offset_ex( ExPolygons(layerm.slices), - scale_(layerm.region()->config.Hatch_Spacing.value));
		//layerm.slices.set(layerm_slices_surfaces, stInternal);
        SurfaceCollection top;
        if (upper_layer != NULL) {
            ExPolygons upper_slices;
            upper_slices = upper_layer->regions[region_id]->slices;
			if(!upper_slices.empty())
            top.append(diff_ex(layerm_slices_surfaces, upper_slices, true),stTop);
        } else {
           
            top = layerm.slices;
            for (Surface &s : top.surfaces) s.surface_type = stTop;
        }
    
        // find bottom surfaces (difference between current surfaces
        // of current layer and lower one)
        SurfaceCollection bottom;
		ExPolygons lower_slices;
        if (lower_layer != NULL) {
			lower_slices=lower_layer->regions[region_id]->slices; 
            // If we have soluble support material, don't bridge. The overhang will be squished against a soluble layer separating
            // the support from the print.
		 if(!lower_slices.empty())
		 
		 {			 
       //-surfaceType surface_type_bottom =stBottom;
          
       //-condition ? expression1 : expression2
   
		ExPolygons check_slope= diff_ex(layerm_slices_surfaces, offset_ex(lower_slices,oh_permissible));// (it - 1)->slices);
	    surface_type_bottom =check_slope.empty()? stBottom :stSupport;
		bottom.set(diff_ex(layerm_slices_surfaces,  lower_slices, true),surface_type_bottom);
		
		
		// extra
		//{ 
		//ExPolygons PP=object.support_structure_Polygons[region_id];
		//append_to(PP,to_expolygons(bottom.surfaces));
		//object.support_structure_Polygons[region_id]=PP;

		}
     
        } else 
        {
        // -if no lower layer, all surfaces of this one are solid
        // -we clone surfaces because we're going to clear the slices collection
            bottom=layerm.slices;
        // -if we have raft layers, consider bottom layer as a bridge
        // -just like any other bottom surface lying on the void
        /* SurfaceType surface_type_bottom =
                (object.config.raft_layers.value > 0 && object.config.support_material_contact_distance.value > 0)
                ? stBottomBridge
                : stBottom;*/
        surface_type_bottom =stSupport; 
		for (Surface &s : bottom.surfaces) s.surface_type = surface_type_bottom;
        // -surface_type_bottom =stBottom;
		// -object.support_structure_Polygons[region_id].append(bottom);
		// -ExPolygons PP=object.support_structure_Polygons[region_id];
		// -append_to(PP,to_expolygons(bottom.surfaces));
		// -object.support_structure_Polygons[region_id]=PP;
		// -include the base  layer..
		}
	if(surface_type_bottom ==stSupport)	
	object.support_structure_Polygons[region_id].append(bottom);

   
	    // -ExPolygons check_slope;
	
		// -boost::nowide::cout << "area: "<< layerm_slices_surfaces.front().area() << std::endl;

        // -now, if the object contained a thin membrane, we could have overlapping bottom
        // -and top surfaces; let's do an intersection to discover them and consider them
        // -as bottom surfaces (to allow for bridge detection)
        if (!top.empty() && !bottom.empty()) {
            const ExPolygons top_polygons = (STDMOVE(top));
            top.clear();
            top.append(
            // TODO: maybe we don't need offset2?
            diff_ex(top_polygons, bottom, true),
            stTop
            );
        
    
        // -save surfaces to layer
        
            boost::lock_guard<boost::mutex> l(layerm._slices_mutex);
            layerm.slices.clear();
            layerm.slices.append(STDMOVE(top));
            layerm.slices.append(STDMOVE(bottom));
    
        // -find internal surfaces (difference between top/bottom surfaces and others)
            
            ExPolygons topbottom = top; append_to(topbottom, (ExPolygons)bottom);
    
            layerm.slices.append(
                    // TODO: maybe we don't need offset2?
                    diff_ex(layerm_slices_surfaces, topbottom, true),
                    stInternal
                );
	}
        
    
        #ifdef SLIC3R_DEBUG
        printf("  layer %d has %d bottom, %d top and %d internal surfaces\n",
            this->id(), bottom.size(), top.size(),
            layerm.slices.size()-bottom.size()-top.size());
        #endif
    
        
            /*  Fill in layerm->fill_surfaces by trimming the layerm->slices by the cummulative layerm->fill_surfaces.
                Note: this method should be idempotent, but fill_surfaces gets modified
                in place. However we're now only using its boundaries (which are invariant)
                so we're safe. This guarantees idempotence of prepare_infill() also in case
                that combine_infill() turns some fill_surface into VOID surfaces.  */
            //const Polygons fill_boundaries = layerm.fill_surfaces;
            // layerm.fill_surfaces.clear();
            // No other instance of this function is writing to this layer, so we can read safely.
          /*  for (const Surface &surface : layerm.slices.surfaces) {
                // No other instance of this function modifies fill_surfaces.
                layerm.fill_surfaces.append(surface.expolygon,
                    //intersection_ex(surface, fill_boundaries),
                    surface.surface_type
                );
            }*/
        //layerm.fill_surfaces= layerm.slices;

   }
     // -layerm.MakeGrids();
	 std::cout << "surface  done: " << std::endl;
}

///Iterates over all LayerRegions and invokes LayerRegion->process_external_surfaces
void
Layer::process_external_surfaces()
{
    for (LayerRegion* &layerm : this->regions)
        layerm->process_external_surfaces();
}

void
Layer::detect_support_layers()
{
   /* PrintObject &object = *this->object();
     
	float oh_angle=0.01745*(object.config.support_material_angle.value+1); //  radians
	coord_t oh_permissible =scale_(this->height/tan(oh_angle)); // mm
	for (size_t region_id = 0; region_id < this->regions.size(); ++region_id) {
        LayerRegion &layerm = *this->regions[region_id];
        
       
        Layer* const &upper_layer = this->upper_layer;
        Layer* const &lower_layer = this->lower_layer;
    
        // collapse very narrow parts (using the safety offset in the diff is not enough)
        // TODO: this offset2 makes this method not idempotent (see #3764), so we should
        // move it to where we generate fill_surfaces instead and leave slices unaltered
        const float offs = 1;//layerm.flow(frExternalPerimeter).scaled_width() / 10.f;

        //const
		ExPolygons layerm_slices_surfaces = layerm.slices;

        // find top surfaces (difference between current surfaces
        // of current layer and upper one)
        SurfaceCollection top;
        if (upper_layer != NULL) {
            ExPolygons upper_slices;
            if (object.config.interface_shells.value) {
                const LayerRegion* upper_layerm = upper_layer->get_region(region_id);
                boost::lock_guard<boost::mutex> l(upper_layerm->_slices_mutex);
                upper_slices = upper_layerm->slices;
            } else {
                upper_slices = upper_layer->slices;
            }
            upper_slices = upper_layer->slices.expolygons;
            top.append(
                
                    diff_ex(layerm_slices_surfaces, upper_slices, true),
                 
                
                stTop
            );
        } else {
            // if no upper layer, all surfaces of this one are solid
            // we clone surfaces because we're going to clear the slices collection
            top = layerm.slices;
            for (Surface &s : top.surfaces) s.surface_type = stTop;
        }
    
        // find bottom surfaces (difference between current surfaces
        // of current layer and lower one)
        SurfaceCollection bottom;
        if (lower_layer != NULL) {
            // If we have soluble support material, don't bridge. The overhang will be squished against a soluble layer separating
            // the support from the print.
            const SurfaceType surface_type_bottom =stBottom;
                //(object.config.support_material.value && object.config.support_material_contact_distance.value == 0)
               // ? stBottom
                //: stBottomBridge;       
            // Any surface lying on the void is a true bottom bridge (an overhang)
        //for (Surface &s : layerm.slices.surfaces) //s.surface_type = surface_type_bottom;
        bottom.append(diff_ex(layerm_slices_surfaces, lower_layer->slices.expolygons, true),
                surface_type_bottom);
        if(oh_permissible > 1) 
		{ ExPolygons check_slope= diff_ex( layerm_slices_surfaces,offset_ex(lower_layer->slices.expolygons, oh_permissible));// (it - 1)->slices);
		if(!check_slope.empty())
		{ // extra
		ExPolygons PP=object.support_structure_Polygons[region_id];
		append_to(PP,to_expolygons(bottom.surfaces));
		object.support_structure_Polygons[region_id]=PP;
		// end extra
		// combine with support of upper layer.
		if (upper_layer != NULL)
		bottom.append(upper_layer->regions[region_id]->support_surfaces);
		// add to lower-layer  support surfaces
		lower_layer->regions[region_id]->support_surfaces.append(diff_ex(union_ex(ExPolygons(bottom)),lower_layer->slices.expolygons), surface_type_bottom);
		 }}
     
        } else {
            // if no lower layer, all surfaces of this one are solid
            // we clone surfaces because we're going to clear the slices collection
            bottom = layerm.slices;
        
            // if we have raft layers, consider bottom layer as a bridge
            // just like any other bottom surface lying on the void
            const SurfaceType surface_type_bottom =
                (object.config.raft_layers.value > 0 && object.config.support_material_contact_distance.value > 0)
                ? stBottomBridge
                : stBottom;
            for (Surface &s : bottom.surfaces) s.surface_type = surface_type_bottom;
         
		ExPolygons PP=object.support_structure_Polygons[region_id];
		append_to(PP,to_expolygons(bottom.surfaces));
		object.support_structure_Polygons[region_id]=PP;
		  // include the base  layer..
		}
    
	//ExPolygons check_slope;
	
		// boost::nowide::cout << "area: "<< layerm_slices_surfaces.front().area() << std::endl;

        // now, if the object contained a thin membrane, we could have overlapping bottom
        // and top surfaces; let's do an intersection to discover them and consider them
        // as bottom surfaces (to allow for bridge detection)
        if (!top.empty() && !bottom.empty()) {
            const Polygons top_polygons = to_polygons(STDMOVE(top));
            top.clear();
            top.append(
                // TODO: maybe we don't need offset2?
                offset2_ex(diff(top_polygons, bottom, true), -offs, offs),
                stTop
            );
        }
    
        // save surfaces to layer
        {
            boost::lock_guard<boost::mutex> l(layerm._slices_mutex);
            layerm.slices.clear();
            layerm.slices.append(STDMOVE(top));
            layerm.slices.append(STDMOVE(bottom));
    
            // find internal surfaces (difference between top/bottom surfaces and others)
            {
                ExPolygons topbottom = top; append_to(topbottom, (ExPolygons)bottom);
    
                layerm.slices.append(
                    // TODO: maybe we don't need offset2?
                    offset_ex(
                        diff_ex(layerm_slices_surfaces, topbottom, true),
                        0
                    ),
                    stInternal
                );
            }
        }
    
        #ifdef SLIC3R_DEBUG
        printf("  layer %d has %d bottom, %d top and %d internal surfaces\n",
            this->id(), bottom.size(), top.size(),
            layerm.slices.size()-bottom.size()-top.size());
        #endif
    
        {
            
            const Polygons fill_boundaries = layerm.fill_surfaces;
            layerm.fill_surfaces.clear();
            // No other instance of this function is writing to this layer, so we can read safely.
            for (const Surface &surface : layerm.slices.surfaces) {
                // No other instance of this function modifies fill_surfaces.
                layerm.fill_surfaces.append(
                    intersection_ex(surface, fill_boundaries),
                    surface.surface_type
                );
            }
        }
  
   }*/
	
}


 

void
Layer::write_svg(SVG *sv)
{
	//std::cout << "writingnow\n" << std::endl ;
	PrintObject &object = *this->object();
    std::string outputfile("Layer");
	
   ExPolygon expp;
   std::ostringstream num;
   num << this->id();
   outputfile=outputfile+num.str();
   const char *ch= outputfile.c_str();
   sv->layer_section_start(ch);
     //SVG *sv  = new SVG(outputfile+num.str()+std::string(".svg")); // allocate memory
   sv->draw( Point(0,0 ), std::string("red"), scale_(0.1));
   sv->draw( Point(0,0 ), std::string("black"), scale_(50));
   
   FOREACH_LAYERREGION(this, layerm) {
   SurfaceCollection gridcollection=(*layerm)->slices;

    for (Surfaces::const_iterator surface =gridcollection.surfaces.begin(); surface != gridcollection.surfaces.end(); ++surface) 
  {
      SurfaceType  type=surface->surface_type;
	   expp= surface->expolygon;
	if(type==stTop)
		sv->draw_outline(expp,std::string ("indigo"), 0.2);
	else if(type==stBottom)
		sv->draw_outline(expp,std::string ("red"), 0.2);
	else
	 sv->draw_outline(expp, std::string ("green"), 0.2);	 
				
    }
	
			
    for (Lines::const_iterator it= (*layerm)->scan_vectors.begin(); it != (*layerm)->scan_vectors.end(); ++it) 
         sv->draw(*it, std::string("green"), 0.2);
  
  /*for (size_t region_id = 0; region_id < this->regions.size(); ++region_id)
  { ExPolygons epp=object.support_structure_Polygons[region_id];
    //epp=union_ex(offset_ex(epp,200));
	for (  ExPolygons::const_iterator  e =epp.begin();  e!= epp.end(); ++e) 
			sv->draw_outline(*e, std::string ("red"), 1.0); }*/
	
 gridcollection=(*layerm)->support_surfaces;

    for (Surfaces::const_iterator surface =gridcollection.surfaces.begin(); surface != gridcollection.surfaces.end(); ++surface) 
  {

	 expp= surface->expolygon;
	
	 sv->draw_outline(expp,std::string ("blue"), 0.6);
		 
				
    }   
	}
	sv->layer_section_end();
//delete  sv;
//std::cout << "writingEnd\n" << std::endl ;
}
void
Layer::write_svg_layers()
{
	//std::cout << "writingnow\n" << std::endl ;
   PrintObject &object = *this->object();
   std::string outputfile("Layer");
	
	ExPolygon expp;
    std::ostringstream num;
    num << this->id();
  // outputfile=outputfile+num.str();
   //const char *ch= outputfile.c_str();
 //sv->layer_section_start(ch);
   SVG *sv  = new SVG(outputfile+num.str()+std::string(".svg")); // allocate memory
   sv->draw( Point(0,0 ), std::string("red"), scale_(0.1));
   sv->draw( Point(0,0 ), std::string("none"), scale_(80));
   
   FOREACH_LAYERREGION(this, layerm) {
   SurfaceCollection gridcollection=(*layerm)->fill_surfaces;

   /* for (Surfaces::const_iterator surface =gridcollection.surfaces.begin(); surface != gridcollection.surfaces.end(); ++surface) 
  {
      SurfaceType  type=surface->surface_type;
	   expp= surface->expolygon;
	if(type==stTop)
		sv->draw_outline(expp,std::string ("indigo"), 0.2);
	else if(type==stBottom)
		sv->draw_outline(expp,std::string ("red"), 0.4);
	//else
	 //sv->draw_outline(expp, std::string ("green"), 0.2);	 
				
    }*/
	
	  
	for (ExPolygons::const_iterator it= (*layerm)->skin.begin(); it != (*layerm)->skin.end(); ++it) 
         //sv->draw(*it, std::string("green"), 0.2);
	   
	   {  ExPolygon P =*it;
	      P.translate(object._copies_shift.x,object._copies_shift.y);
		   sv->draw_outline(P,std::string ("black"), 0.2);
		   }
		 	
    //for (Lines::const_iterator it= (*layerm)->scan_vectors.begin(); it != (*layerm)->scan_vectors.end(); ++it) 
         //sv->draw(*it, std::string("green"), 0.2);
  
  for (ExtrusionPathPtrs::const_iterator it= (*layerm)->fills.ExposureVectors.begin(); it != (*layerm)->fills.ExposureVectors.end(); ++it) 
  {  ExtrusionRole role=(*it)->role;
     //for (Lines::const_iterator ir= (*it)->exposure_vectors.begin(); ir != (*it)-> exposure_vectors.end(); ++ir) 
     
 
	if(role==ercoreInfill)
	  sv->draw((*it)->translated(object._copies_shift.x,object._copies_shift.y), std::string("green"), 0.2);
    else if(role==eroverhangInfill)
	   sv->draw((*it)->translated(object._copies_shift.x,object._copies_shift.y), std::string("red"), 0.2);
   
    else if(role==erover_overhangInfill)
	   sv->draw((*it)->translated(object._copies_shift.x,object._copies_shift.y), std::string("black"), 0.2);
  
    else if(role==erTopSolidInfill)
	   sv->draw((*it)->translated(object._copies_shift.x,object._copies_shift.y), std::string("indigo"), 0.2);
    else if(role==erSupportMaterial)
	   sv->draw((*it)->translated(object._copies_shift.x,object._copies_shift.y), std::string("red"), 0.2);
  
   
  
  
  
  
  }
  
   
  
  /*for (size_t region_id = 0; region_id < this->regions.size(); ++region_id)
  { ExPolygons epp=object.support_structure_Polygons[region_id];
    //epp=union_ex(offset_ex(epp,200));
	for (  ExPolygons::const_iterator  e =epp.begin();  e!= epp.end(); ++e) 
			sv->draw_outline(*e, std::string ("red"), 1.0); }*/
	
    gridcollection=(*layerm)->support_surfaces;

    for (Surfaces::const_iterator surface =gridcollection.surfaces.begin(); surface != gridcollection.surfaces.end(); ++surface) 
  { expp= surface->expolygon;
	expp.translate(object._copies_shift.x,object._copies_shift.y);
	sv->draw_outline(expp,std::string ("blue"), 0.6);
  }   
	}
//sv->layer_section_end();
delete  sv;
//std::cout << "writingEnd\n" << std::endl ;
}
}
