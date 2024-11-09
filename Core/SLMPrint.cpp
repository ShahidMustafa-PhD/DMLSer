#define DEBUG_FillDMLSisland
#include "SLMPrint.hpp"
//#include "GCode.hpp"

#include "ClipperUtils.hpp"
#include "ExtrusionEntity.hpp"
#include "Fill/Fill.hpp"
#include "Geometry.hpp"
#include "Surface.hpp"
#include <iostream>
#include <complex>
#include <cstdio>
#include<string>
#include <climits>
#include <cstdlib>
#include <iostream>
namespace Slic3r {

void
SLMPrint::slice()
{
	
 
    TriangleMesh mesh = this->model->mesh();
    mesh.repair();
   
    // align to origin taking raft into account
        this->bb = mesh.bounding_box();
         
      if (this->config.raft_layers > 0) 
      {
        this->bb.min.x -= this->config.raft_offset.value;
        this->bb.min.y -= this->config.raft_offset.value;
        this->bb.max.x += this->config.raft_offset.value;
        this->bb.max.y += this->config.raft_offset.value;
      }
   
    mesh.translate(0, 0, -bb.min.z);
    this->bb.translate(0, 0, -bb.min.z);
    
    // if we are generating a raft, first_layer_height will not affect mesh slicing
    const float lh       =this->config.layer_height.value;
    const float first_lh = this->config.first_layer_height.value;
    std::cout << "Layer Height: " << lh <<std::endl;
    std::cout << "Layer Height: " << this->config.island_L.value  <<std::endl;
    std::cout << "Layer Height: " << this->config.island_H.value <<std::endl;
	
    // generate the list of Z coordinates for mesh slicing
    // (we slice each layer at half of its thickness)
    this->layers.clear();
    {
        const float first_slice_lh = (this->config.raft_layers > 0) ? lh : first_lh;
        this->layers.push_back(Layer(first_slice_lh/2, first_slice_lh));
    }
    while (this->layers.back().print_z + lh/2 <= mesh.stl.stats.max.z) {
        this->layers.push_back(Layer(this->layers.back().print_z + lh/2, this->layers.back().print_z + lh));
    }
    
    // perform slicing and generate layers
	bool adaptive=0;
	float LT_min=0.04;
	float LT_max=0.2;
	float step= abs(LT_max-LT_min)/3;

	if (adaptive)
		
	{   
	std::vector<float> slice_z_Adapt; 
	std::vector<ExPolygons> slices_Adapt;
    TriangleMeshSlicer<Z>(&mesh).slice(slice_z_Adapt, &slices_Adapt);
    }
        
        
    {
        std::vector<float> slice_z;
        for (size_t i = 0; i < this->layers.size(); ++i)
        slice_z.push_back(this->layers[i].slice_z);
        //- here it slises the model and saves as expolygons...
        std::vector<ExPolygons> slices;
        TriangleMeshSlicer<Z>(&mesh).slice(slice_z, &slices);
        
        for (size_t i = 0; i < slices.size(); ++i)
            this->layers[i].slices.expolygons = slices[i];
		
	// - assignes slices of each layer
	// -slicing is complete   and contours + holes available  for filling
    }
	//Make this call before parrallelizing..  otherwise  ıt will not work..
	// very important note
	  
	if(this->config.SLM_ChessBoard.value)
		  this->MakeGrids();
	//this->MakeGrids(); //  generates  clipped grids and saves as expolygones

	std::cout << "Fill_Pattern: " << this->config.fill_pattern.value << std::endl;
	std::unique_ptr<Fill> fill(Fill::new_from_type(this->config.fill_pattern.value));
    fill->bounding_box.merge(Point::new_scale(bb.min.x, bb.min.y));
    fill->bounding_box.merge(Point::new_scale(bb.max.x, bb.max.y));
    fill->min_spacing   = this->config.Hatch_Spacing.value;//this->config.get_abs_value("infill_extrusion_width", this->config.layer_height.value);
    fill->angle         = Geometry::deg2rad(this->config.fill_angle.value);
    fill->density       = this->config.fill_density.value/100;
	fill->hatchspacing  =this->config.Hatch_Spacing.value;
	fill->overlape_offset=this->config.SLM_overlap.value;
	fill->island_area_max=scale_(100)*scale_(100);
	
	if(this->config.SLM_ChessBoard.value)
	fill->island_area_max=0.95*scale_(this->config.island_H.value)*scale_(this->config.island_L.value);
    //-defined in slic3r.h  for multithreading...
    std::cout << "HatchSpacing: " << this->config.Hatch_Spacing.value <<std::endl;
    //-makes  pillars  for support material....
    std::cout << "SLMOverLap: " << this->config.SLM_overlap.value <<std::endl;
    parallelize<size_t>(  
           0,
           this->layers.size()-1,
           boost::bind(&SLMPrint::_infill_layer, this, _1, fill.get()),
           this->config.threads.value
            ); //   -note this binding exec   infill   from 1 to last layer..


    this->sm_pillars.clear();
    ExPolygons overhangs;
		
	//-boost::nowide::cout << "   Support Material? " << this->config.support_material<<std::endl;
    if(this->config.support_material)
	    {   //------------
	    if (this->config.raft_layers > 0) 
	    {
            //-ExPolygons raft = this->layers.front().slices + overhangs;  // take support material into account
            //raft = offset_ex(raft, scale_(this->config.raft_offset));
            for (int i = this->config.raft_layers; i >= 1; --i)
             {
              this->layers.insert(this->layers.begin(), Layer(0, first_lh + lh * (i-1)));
           // this->layers.front().slices = raft;
             }
        
        // -prepend total raft height to all sliced layers
        for (size_t i = this->config.raft_layers; i < this->layers.size(); ++i)
        this->layers[i].print_z += first_lh + lh * (this->config.raft_layers-1);
        }
	//---------------------
	
    // -flatten and merge all the overhangs
    {
    Polygons pp;
	Polygons check_slope;
	float oh_angle=0.01745*(this->configfull.support_material_angle.value+1); //  radians
	float oh_permissible=this->config.layer_height.value/tan(oh_angle); // mm
    //boost::nowide::cout << "OH Permissible " << oh_permissible <<std::endl;
	for (std::vector<Layer>::const_iterator it = this->layers.begin()+1; it != this->layers.end(); ++it)
	{
	check_slope= diff(it->slices,offset((it - 1)->slices, scale_(oh_permissible)));// (it - 1)->slices);
	     
	  if (!check_slope.empty())
	  {
	  check_slope.clear();
	  pp += diff(it->slices,(it - 1)->slices);}
	  }
	//offset((it - 1)->slices, -scale_(3))
    // pp +=layers.front().slices;
    overhangs = union_ex(pp);
    }
        
    // -generate points following the shape of each island
    Points pillars_pos;
    const coordf_t spacing = scale_(this->config.support_material_spacing);
    const coordf_t radius  = scale_(this->sm_pillars_radius());
	const coordf_t  ofset= scale_(0.1);
       
	for (ExPolygons::const_iterator it = overhangs.begin(); it != overhangs.end(); ++it)       {
    // -leave a radius/2 gap between pillars and contour to prevent lateral adhesion
    //boost::nowide::cout << "Overhangs3\n " << std::endl;
	this->Conto=it->contour; // must call befote bounding
	this->BoundRect();
	for(coord_t ycc=(Rmin.y-spacing); ycc < (Rmax.y+spacing); ycc+=spacing)
        {  
        ycc+=ofset;//scale_(this->config.SLM_overlap.value);//HatchDistanInt;
        for(coord_t xcc=(Rmin.x-spacing); xcc < (Rmax.x+spacing); xcc+=spacing)
		  {
		   xcc+=ofset;
	       if(this->Conto.contains( Point(xcc,ycc)))
		   pillars_pos.push_back(Point(xcc,ycc));
	      }
	    }
		
		/*for (float inset = radius/4;; inset += spacing) {  //inset = radius * 1.5;
                // inset according to the configured spacing
                Polygons curr = offset(*it, -inset);
				//boost::nowide::cout << "Number of Currs:\n " << curr.size()<< std::endl;
                if (curr.empty()) break;
             
                // generate points along the contours
                for (Polygons::const_iterator pg = curr.begin(); pg != curr.end(); ++pg) {
                    Points pp=pg->equally_spaced_points(spacing);
					//boost::nowide::cout << "Number of Points:\n " << pp.size()<< std::endl;
                    for (Points::const_iterator p = pp.begin(); p != pp.end(); ++p)
                        pillars_pos.push_back(*p);
                }
            }*/
           }     //  upto here  we have generated position of each piller in XY plane. Not Z.
        //boost::nowide::cout << "Number of positions:\n " << pillars_pos.size()<< std::endl;
        // for each pillar, check which layers it applies to
        for (Points::const_iterator p = pillars_pos.begin(); p != pillars_pos.end(); ++p) 
        {
            SupportPillar pillar(*p);
            bool object_hit = false;
            
            // -check layers top-down
                for (int i = this->layers.size()-1; i >= 0; --i) 
                {
                // -check whether point is void in this layer
                     if (!this->layers[i].slices.contains(*p)) 
                     {
                    // -no slice contains the point, so it's in the void
					// -ıt must be in the void to be eligible for piller..
                    if (pillar.top_layer > 0) {
                        // -we have a pillar, so extend it
                        pillar.bottom_layer = i ;//+ this->config.raft_layers;
                     } else if (object_hit) {
                        // -we don't have a pillar and we're below the object, so create one
                        pillar.top_layer = i ;//+ this->config.raft_layers;
                      } 
                    } else {
                    if (pillar.top_layer > 0) {
                        // -we have a pillar which is not needed anymore, so store it and initialize a new potential pillar
                        this->sm_pillars.push_back(pillar);
                        pillar = SupportPillar(*p);
                    }
                    object_hit = true;
                }
            }
            if (pillar.top_layer > 0) this->sm_pillars.push_back(pillar);
        }
    
	  Make_Support();
	  }
   
    // generate a solid raft if requested
    // (do this after support material because we take support material shape into account)
    /*if (this->config.raft_layers > 0) {
        //ExPolygons raft = this->layers.front().slices + overhangs;  // take support material into account
        //raft = offset_ex(raft, scale_(this->config.raft_offset));
        for (int i = this->config.raft_layers; i >= 1; --i) {
            this->layers.insert(this->layers.begin(), Layer(0, first_lh + lh * (i-1)));
           // this->layers.front().slices = raft;
        }
        
        // prepend total raft height to all sliced layers
        for (size_t i = this->config.raft_layers; i < this->layers.size(); ++i)
            this->layers[i].print_z += first_lh + lh * (this->config.raft_layers-1);
    }*/
	
      
}

void
SLMPrint::_infill_layer(size_t i, const Fill* _fill)
{ 
  	
    Layer &layer = this->layers[i];
 
    //const Polygons infill = offset(layer.slices, -scale_(shell_thickness));
    
    // Generate solid infill
    //layer.solid_infill << diff_ex(infill, internal, true);

    
        std::unique_ptr<Fill> fill(_fill->clone());
        fill->layer_id = i;
        fill->z        = layer.print_z;
        
		//ExPolygons  eppss;
		//std::string pdf;
        //const ExPolygons internal_ex = intersection_ex(infill, internal);
        //  we can choose between following two...
       Lines cleanlines;
       double fillangle= this->config.Hatch_Spacing.value;
if(this->config.SLM_ChessBoard.value)
		{  
	const std::vector<ExPolygons> &elices = layer.gridpolygons;//gridpolygons;
	for (std::vector<ExPolygons>::const_iterator it = elices.begin(); it != elices.end(); ++it)
			{
	for (ExPolygons::const_iterator et=it->begin(); et != it->end(); ++et) 
			{
               Polylines polylines = fill->fill_surface(Surface(stInternal, *et));  // here  it usese.....
	for (Polylines::const_iterator pl=polylines.begin(); pl !=polylines.end(); ++pl) 
	{  cleanlines=pl->scan_lines() ;
			Polygons pgs;
		for (ExPolygons::const_iterator ct = layer.slices.expolygons.begin(); ct != layer.slices.expolygons.end(); ++ct)
				{	
				pgs.push_back(ct->contour);
				Lines plines=intersection_ln(cleanlines,pgs);
                Polygons hols=ct->holes;
			    if(!hols.empty())
			    layer.mGridLines.push_back(diff_ln(plines, hols));
			    else 
	           layer.mGridLines.push_back(plines);					
				 }
	}	 
            
		}
			}
		}
		else //if no grids
		{for (ExPolygons::const_iterator it = layer.slices.expolygons.begin(); it != layer.slices.expolygons.end(); ++it) {
            Polylines polylines = fill->fill_surface(Surface(stInternal, *it));  // here  it usese.....
           //layer.infill.append(polylines, templ);
			//boost::nowide::cout << "insideel: \n" << std::endl;
				for (Polylines::const_iterator pl=polylines.begin(); pl !=polylines.end(); ++pl) 
			{
			cleanlines=pl->scan_lines();
				
			 layer.mGridLines.push_back(cleanlines);
			}
		}
		}

		//  this one is grid fills...
		if(this->configfull.perimeters.value) 
	     layer.perimeters.push_back( diff(layer.slices,offset(layer.slices, -scale_(this->config.Hatch_Spacing/2))));  
	 //printf("filling done...  done:\n");

}




coordf_t
SLMPrint::sm_pillars_radius() const
{
    //coordf_t radius = this->config.support_material_extrusion_width.get_abs_value(this->config.support_material_spacing)/2;
   // if (radius == 0) 
	  coordf_t  radius = this->config.support_material_spacing / 1.2; // auto
      return radius;
}

std::string
SLMPrint::polygon_d(const Polygon &polygon) const
{
    const Sizef3 size = this->bb.size();
    std::ostringstream d;
    //d << "M ";
    for (Points::const_iterator p = polygon.points.begin(); p != polygon.points.end(); ++p) {
        d << unscale(p->x) << " ";// - this->bb.min.x << " ";
        d << unscale(p->y) << " ";//  <<- this->bb.min.y) << " ";  // mirror Y coordinates as SVG uses downwards Y
    }
   // d << "z";
    return d.str();
}

std::string
SLMPrint::polygon_d(const ExPolygon &expolygon) const
{
    std::string pd;
	
    const Polygons pp = expolygon;
    for (Polygons::const_iterator mp = pp.begin(); mp != pp.end(); ++mp) 
        pd += this->polygon_d(*mp) + "\n";
    return pd;
}

std::string
SLMPrint::polygon_d(const Polyline &polyline) const
{
    const Sizef3 size = this->bb.size();
    std::ostringstream d;
    //d << "M ";
	//for (Polylines::const_iterator pp = polylines.begin(); pp != polylines.end(); ++pp){
    for (Points::const_iterator p = polyline.points.begin(); p != polyline.points.end(); ++p) {
        d << unscale(p->x) - this->bb.min.x << " ";
        d << size.y - (unscale(p->y) - this->bb.min.y) << " ";  // mirror Y coordinates as SVG uses downwards Y
    }
    //d << "Z";
    return d.str();
}


void
SLMPrint::write_vectors(const std::string &outputfile) const
{
    const Sizef3 size = this->bb.size();

	
    FILE* f = fopen(outputfile.c_str(), "w");
  
    for (size_t i = 0; i < this->layers.size(); ++i) {
        const Layer &layer = this->layers[i];
      
            fprintf(f,"%s\n","L");
			
	 for (std::vector<Lines>::const_iterator it = layer.mGridLines.begin();it != layer.mGridLines.end(); ++it)
		 {   
              	 for (Lines::const_iterator e = it->begin(); e != it->end(); ++e) {
  
				   fprintf(f,"%s\n",e->wkt_SLM().c_str());
				}
            }
			
			 const std::vector<Polygons> &perimetrs = layer.perimeters;
            for (std::vector<Polygons>::const_iterator it = perimetrs.begin(); it != perimetrs.end(); ++it) {
                for (Polygons::const_iterator itt = it->begin(); itt != it->end(); ++itt) {
			   std::string pd = this->polygon_d(*itt);
                fprintf(f,"%s",pd.c_str());
				}
               
            }

	
	}

}

void
SLMPrint::write_Polygons(const std::string &outputfile) const
{
    const Sizef3 size = this->bb.size();
    const double support_material_radius = sm_pillars_radius();
	
    FILE* f = fopen(outputfile.c_str(), "w");
    //boost::nowide::cout << "Number of pillers: " << sm_pillars.size() << std::endl; 
    
    for (size_t i = 0; i < this->layers.size(); ++i)

		{
        const Layer &layer = this->layers[i];
      
            fprintf(f,"%s\n","L");
			//const ExPolygons &grid = layer.mGrid;
            const ExPolygons &slices = layer.slices.expolygons;
            for (ExPolygons::const_iterator it = slices.begin(); it != slices.end(); ++it) {
                std::string pd = this->polygon_d(*it);
                //boost::nowide::cout << "Area=\n " << it->area()<< std::endl;
				fprintf(f,"%s",pd.c_str());
               
            }
		    fprintf(f,"%s\n","S");
		   const  std::vector<Polygons> &sup = layer.support;
            for (std::vector<Polygons>::const_iterator it = sup.begin(); it != sup.end(); ++it) {
                for (Polygons::const_iterator itt = it->begin(); itt != it->end(); ++itt) {
				std::string pd = this->polygon_d(*itt);
                fprintf(f,"%s\n",pd.c_str());
				}
            }
       

	
	 /*for (std::vector<SupportPillar>::const_iterator it = this->sm_pillars.begin(); it != this->sm_pillars.end(); ++it) {
                if (!(it->top_layer >= i && it->bottom_layer <= i)) continue;
            
 
            float radius =fminf(support_material_radius,
			(it->top_layer - i + 1) *this->config.layer_height.value);
            coordf_t x_c=unscale(it->x);
			coordf_t y_c=unscale(it->y);
			
			  //std::string pd=this->_SVG_path_d(Make_Support(1,x_c, y_c));
			  //fprintf(f,"%s \n",pd.c_str());
			
				
            }*/

	
	}

}




/*void 
SLMPrint::RectGrids()
{   //std::vector<Slic3r::Point>  
 ClipperLib::Path grid;
 
//mypoints.clear();
      rectangularGrids.clear();
	   ClipperLib::Paths LrectangularGrid;
	//printf("Making Grid");
   // rectangularGrids.clear();
	//BoundingBox bbox = _expolygon.contour.bounding_box();
     coord_t offset1=1;//scale_(rand()%10);// random rect size between 0 and 2mm..
     coord_t island_HInt=ClipperLib::cInt(scale_(island_H)+offset1);
     coord_t island_LInt=ClipperLib::cInt(scale_(island_L)+offset1);
     coord_t HatchDistanInt =ClipperLib::cInt(scale_(HatchDistan));
	 //this->bb.min.x;
	 //printf(" %d %d %d %d",this->bb.min.x,this->bb.min.y,this->bb.max.x,this->bb.max.y);
    for(ClipperLib::cInt ycc=Rmin.Y; ycc < Rmax.Y; ycc+=island_HInt )
    {  //ycc=ycc+HatchDistanInt;
        for( ClipperLib::cInt xcc=Rmin.X; xcc < Rmax.X; xcc+=island_LInt )
        {  // xcc=xcc+HatchDistanInt;
	      //mypoints+= (mypoints, Point(xcc,ycc));
		  //rectangularGrids+=(Polygon());
		  grid<< ClipperLib::IntPoint(xcc,ycc)<< ClipperLib::IntPoint(xcc,ycc+island_HInt) << ClipperLib::IntPoint(xcc+island_LInt,ycc+island_HInt) << ClipperLib::IntPoint(xcc+island_HInt,ycc) ;
		  	 //mypoints.push_back(Slic3r::Point(xcc,ycc));// 
	         //mypoints.push_back(Slic3r::Point(xcc,ycc+island_HInt));
			//mypoints.push_back(Slic3r::Point(xcc+island_LInt,ycc+island_HInt));
			//mypoints.push_back(Slic3r::Point(xcc+island_HInt,ycc)) ;
			//Polygon gridB=Polygon( mypoints);
			//Polygon   MlPoints(mypoints);
            //LrectangularGrid=Polygon(ClipperPath_to_Slic3rMultiPoint(grid));
		    LrectangularGrid.push_back(grid);
       

           grid.clear();

        }
    }
 ExPolygons menq=ClipperPaths_to_Slic3rExPolygons(LrectangularGrid);
 //rectangularGrids=ClipperPaths_to_Slic3rMultiPoints(LrectangularGrids));
  rectangularGrids= to_polygons( menq);
}*/

 void
 SLMPrint::write_grid(const std::string &outputfile) const
 {
	 const Sizef3 size = this->bb.size();
    const double support_material_radius = sm_pillars_radius();
	printf("Writing Grid");
    FILE* ff = fopen(outputfile.c_str(), "w");
    //layer.mGrid.push_back(epp);
	//const Layer &layer = this->layers[1];
	 /*const ExPolygons &grid = layer.mGrid;
          // const ExPolygons &slices = layer.mGrid.expolygons;
		   for (ExPolygons::const_iterator it = layer.mGrid.begin(); it != layer.mGrid.end(); ++it){
            for (ExPolygon::const_iterator pt = it->begin(); pt != pt->end(); ++pt) {
                std::string pd = this->_SVG_path_d(*pt);
				//fprintf(f,"%s\n","surface");
                fprintf(ff,"%s\n",pd.c_str());
               
            }
		   }*/
         
		
    /*for (std::vector<std::vector<Polyline>>::const_iterator it = layer.LaserVectors.begin();it != layer.LaserVectors.end(); ++it)
		 {   boost::nowide::cout << "LaserVector size: " << it->size()<< std::endl;
              	  
                for (std::vector<Polyline>::const_iterator e = it->begin(); e != it->end(); ++e) {
                    
                    fprintf(f,"%s\n",e->wkt_DMLS().c_str());
                }
            }*/

		for (size_t i = 0; i < this->layers.size(); ++i) 
        {const Layer &layer = this->layers[i];
       /*fprintf(f,
            "\t<g id=\"layer%zu\" slic3r:z=\"%0.4f\" slic3r:slice-z=\"%0.4f\" slic3r:layer-height=\"%0.4f\">\n",
            i,
            layer.print_z,
            layer.slice_z,
            layer.print_z - ((i == 0) ? 0. : this->layers[i-1].print_z)
        );*/
            fprintf(ff,"%s","L");
       
            const ExPolygons &grids = layer.mGrid.expolygons;
			  //std::vector<Polylines>  &vecs = layer.LaserVectors;
			 //printf("  size of polys=%d\n",vecs.size());
            for (ExPolygons::const_iterator it = grids.begin(); it !=grids.end(); ++it) {
                std::string pd = this->_SVG_path_d(*it);
				//fprintf(f,"%s\n","surface");
                fprintf(ff,"%s\n",pd.c_str());
               
            }
			
		}
         
		
       
    
	
	
	 
 }

	
void SLMPrint::BoundRect()
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
         return;
}

void SLMPrint::RectGrids()
{   
    BoundRect();
    rectangularGrids.clear();
    coord_t offset1=0;//scale_(rand()%10);// random rect size between 0 and 2mm..
    float correctL= std::ceil(std::abs(Rmax.x -Rmin.x)/this->config.island_L.value);
	float correctH= std::ceil(std::abs(Rmax.y -Rmin.y)/this->config.island_H.value);
	if (correctL==0)   correctL=1;
	if (correctH==0)   correctH=1;
	coord_t island_LInt=scale_(std::abs(Rmax.x -Rmin.x)/correctL);
	coord_t island_HInt=scale_(std::abs(Rmax.y -Rmin.y)/correctH);
	//coord_t island_HInt=scale_(this->config.island_H.value);
   // coord_t island_LInt=scale_(this->config.island_L.value);
    coord_t HatchDistanInt=scale_(this->config.Hatch_Spacing.value);
	Points gridp;
	gridp.clear();
			 //mypoints.push_back(Slic3r::Point(xcc,ycc));// 
     
	 //this->bb.min.x;
	//boost::nowide::cout << unscale(this->Rmin.x) << " "<< unscale(this->Rmin.y) << " "<< unscale(this->Rmax.x) << " " << unscale(this->Rmax.y) << "--" <<  this->island_H << std::endl;
	 //printf(" %d %d %d %d",this->Rmin.X,this->Rmin.Y, this->Rmax.X,this->Rmax.Y);
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
	
//boost::nowide::cout << "size of Rectgrids  " << rectangularGrids.size() <<  std::endl;//rectangularGrids=LrectangularGrid;
 //ExPolygons menq=ClipperPaths_to_Slic3rExPolygons(LrectangularGrid);
 //rectangularGrids=ClipperPaths_to_Slic3rMultiPoints(LrectangularGrids));
  //rectangularGrids= to_polygons( menq);
  //boost::nowide::cout << "\ngrid size: "<<  menq.size() << std::endl;
return;
  
  }


void 
SLMPrint::MakeGrids()

{    
	for(size_t i=0; i< layers.size() ; ++i){
	Layer &layer =  this->layers[i];
	layer.offset_Slices << intersection_ex(layer.slices,offset(layer.slices, -scale_(this->config.Hatch_Spacing/2))); 
   
	for (ExPolygons::const_iterator it = layer.offset_Slices.expolygons.begin(); it != layer.offset_Slices.expolygons.end(); ++it) {
        
		Holes.clear();
		Conto=it->contour;
        Holes=it->holes;
		RectGrids();
	    Polygons pp= intersection(rectangularGrids, Conto);
	    layer.gridpolygons.push_back(diff_ex(pp,Holes));
		pp.clear();
	
   }
}

}

/*Polygon
 SLMPrint::DMLSClipperPath_to_Slic3rPolygon(const ClipperLib::Path &input)
{
     Points retva;
	 
    for (ClipperLib::Path::const_iterator pit = input.begin(); pit != input.end(); ++pit)
        retva.push_back(Point( (*pit).X, (*pit).Y ));
	
    return Polygon(retva);
}*/
/*Polygons
 SLMPrint::DMLSClipperPaths_to_Slic3rPolygons(const ClipperLib::Paths &input)
{
     Polygons  retval;
	 
   for (ClipperLib::Paths::const_iterator itt = input.begin(); itt != input.end(); ++itt)
        retval.push_back(DMLSClipperPath_to_Slic3rPolygon(*itt));
    return retval;
}*/

void  
SLMPrint::Make_Support() 
 {
Polygons support_polygons;
Points gridp;
//coordf_t Rad=this->config.support_material_spacing / 1.0;
coordf_t RB= this->config.tooth_base_length.value;
coordf_t RT= this->config.tooth_top_length.value;
coordf_t TH= this->config.tooth_hieght.value;
//coordf_t i_N=  TH/this->config.layer_height.value;
size_t  N_H=0;
size_t  D_N=0;
coordf_t taper_gain= ((RB-RT)/TH)*this->config.layer_height.value; // mm/layer
// boost::nowide::cout << "taper_gain:\n " << taper_gain<< std::endl;
 for (size_t i = 0; i < this->layers.size(); ++i) {
      Layer &layer = this->layers[i];
      support_polygons.clear();
	 for (std::vector<SupportPillar>::const_iterator it = this->sm_pillars.begin(); it != this->sm_pillars.end(); ++it) {
                if (!(it->top_layer >= i && it->bottom_layer <= i)) continue;
				  N_H= (size_t) (it->top_layer - it->bottom_layer)/2;
		
			
	    // boost::nowide::cout << "before:" << Rad <<"\n"<< std::endl;
			  if(i > (it->bottom_layer+N_H)) // if above mid of piller
			  { D_N=it->top_layer-i;} //
		    else if(it->bottom_layer>2)//if not the first layer.
			 {D_N=i-it->bottom_layer;}
		 else  {D_N=50;} // in case first layer radius=material spacing.
		     
     
            float radius =fminf(this->config.support_material_spacing/1.2 ,(RT+taper_gain*D_N));
			//boost::nowide::cout << "after: " << radius<< "\n"<< std::endl;
			coord_t xcc=it->x;
			coord_t ycc=it->y;
			coord_t Ra= scale_(radius/2.0);
			gridp.clear();
                        gridp.push_back(Point(xcc-Ra,ycc-Ra));
	                gridp.push_back(Point(xcc-Ra,ycc+Ra));
	                gridp.push_back(Point(xcc+Ra,ycc+Ra));
                        gridp.push_back(Point(xcc+Ra,ycc-Ra));	
	                support_polygons.push_back(Polygon(gridp));
            }
			 //Polygons grown = offset(layer.slices.contours(), scale_(0.4));
			  //Polygons Cts = layer.slices.contours();
		 Polygons grown;
		 if (i>=this->config.raft_layers )	 // in case not the bottom support.
	   // makes offset from nearby walls..
	   grown =diff(support_polygons,offset(layer.slices.contours(), scale_(this->config.tooth_no_support_offset.value)));
		else grown=support_polygons;
			
			
                     this->layers[i].support.push_back(union_(grown));
                     grown.clear();
	
	}

 
 
 //
    
  
   
       
   
          /*  for (theta = 0.17453; theta < 6.20; theta = theta + (0.08726)*18)  //0.0872=5deg
		{
            x = scale_(R2*cos(theta)+x_c);
            y = scale_(R2*sin(theta)+y_c);
			Ppoint.push_back(Point(x,y));
            }*/
      
       	// boost::nowide::cout << "circle size: "<< Ppoint.size() << std::endl;
       // return Polygon(Ppoint);
		
 }

 void
SLMPrint::Export_SLM_GCode(const std::string &outputfile) const
{   return;
}

	
void
SLMPrint::write_svg(const std::string &outputfile) const
{
    const Sizef3 size = this->bb.size();
    const double support_material_radius = sm_pillars_radius();

    std::string fname=outputfile;

    for (size_t i = 0; i < this->layers.size(); ++i) 
    {
        const Layer &layer = this->layers[i];
        std::ostringstream num;
        num << i;
		
   SVG *sv  = new SVG(outputfile+num.str()+std::string(".svg")); // allocate memory
   sv->draw( Point(0,0 ), std::string("red"), scale_(0.1));
   sv->draw( Point(0,0 ), std::string("none"), scale_(50));
   
          for (std::vector<Lines>::const_iterator it = layer.mGridLines.begin();it != layer.mGridLines.end(); ++it)
          {   
               for (Lines::const_iterator e = it->begin(); e != it->end(); ++e) 
               {
                 sv->draw(*e, std::string("green"), 1);
    /*for (Points::const_iterator ptt = e->points.begin() ; ptt != e->points.end(); ++ptt) 
    { const auto index = std::distance(e->points.begin(), ptt);
		              if (index % 2) {
           sv->draw(Line(*(ptt-1),*ptt), std::string("green"), 1);}	
						}*/
               }
	}
	
	//  Perimeters...
    const std::vector<Polygons> &perimetrs = layer.perimeters;
       for (std::vector<Polygons>::const_iterator it = perimetrs.begin(); it != perimetrs.end(); ++it)   {
          sv->draw_outline(*it, std::string ("indigo"), 0.5);
        }	
			
	// support 
  coordf_t  hatch=scale_(this->config.Hatch_Spacing.value);
  std::vector<Polygons> temporary;
  const std::vector<Polygons> &supp = layer.support;
       for (std::vector<Polygons>::const_iterator it = supp.begin(); it != supp.end(); ++it) 
       { 
        temporary.push_back(*it);
         // temporary.push_back(offset(*it,-hatch));
         //temporary.push_back(offset(*it,-hatch*2));
       }
     
      for (std::vector<Polygons>::const_iterator it = temporary.begin(); it != temporary.end(); ++it) 
      {
              sv->draw_outline(*it, std::string ("red"), 0.3);
      }
	
      delete  sv;
  }
}
 
	

	
	
	
///
    /*for (size_t i = 0; i < this->layers.size(); ++i) {
        const Layer &layer = this->layers[i];
        fprintf(f,
            "\t<g id=\"%d\" slic3r:z=\"%0.4f\" slic3r:slice-z=\"%0.4f\" slic3r:layer-height=\"%0.4f\">\n",
            i,
            layer.print_z,
            layer.slice_z,
            layer.print_z - ((i == 0) ? 0. : this->layers[i-1].print_z)
        );
        
        if (layer.solid) {
            const ExPolygons &slices = layer.slices.expolygons;
            for (ExPolygons::const_iterator it = slices.begin(); it != slices.end(); ++it) {
                std::string pd = this->_SVG_path_d(*it);
                
                fprintf(f,"\t\t<path d=\"%s\" style=\"fill: %s; stroke: %s; stroke-width: %s; />\n",
                    pd.c_str(), "none", "red", "0"
                );
            }
        } else {
            // Perimeters.
            for (ExPolygons::const_iterator it = layer.perimeters.expolygons.begin();
                it != layer.perimeters.expolygons.end(); ++it) {
                std::string pd = this->_SVG_path_d(*it);
                
                fprintf(f,"\t\t<path d=\"%s\" style=\"fill: %s; stroke: %s; stroke-width: %s; fill-type: evenodd\" slic3r:type=\"perimeter\" />\n",
                    pd.c_str(), "white", "black", "0"
                );
            }
            
  
  
            for (ExPolygons::const_iterator it = layer.solid_infill.expolygons.begin();
                it != layer.solid_infill.expolygons.end(); ++it) {
                std::string pd = this->_SVG_path_d(*it);
                
                fprintf(f,"\t\t<path d=\"%s\" style=\"fill: %s; stroke: %s; stroke-width: %s; fill-type: evenodd\" slic3r:type=\"infill\" />\n",
                    pd.c_str(), "white", "black", "0"
                );
            }
            

			
            for (ExtrusionEntitiesPtr::const_iterator it = layer.infill.entities.begin();
                it != layer.infill.entities.end(); ++it) {
                const ExPolygons infill = union_ex((*it)->grow());
                
                for (ExPolygons::const_iterator e = infill.begin(); e != infill.end(); ++e) {
                    std::string pd = this->_SVG_path_d(*e);
                
                    fprintf(f,"\t\t<path d=\"%s\" style=\"fill: %s; stroke: %s; stroke-width: %s; fill-type: evenodd\" slic3r:type=\"infill\" />\n",
                        pd.c_str(), "white", "black", "0"
                    );
                }
            }
        }
        
  
  
        if (i >= (size_t)this->config.raft_layers) {
            // look for support material pillars belonging to this layer
            for (std::vector<SupportPillar>::const_iterator it = this->sm_pillars.begin(); it != this->sm_pillars.end(); ++it) {
                if (!(it->top_layer >= i && it->bottom_layer <= i)) continue;
            
                // generate a conic tip
                float radius = fminf(
                    support_material_radius,
                    (it->top_layer - i + 1) * this->config.layer_height.value
                );
            
                fprintf(f,"\t\t<circle cx=\"%f\" cy=\"%f\" r=\"%f\" stroke-width=\"0\" fill=\"white\" slic3r:type=\"support\" />\n",
                    unscale(it->x) - this->bb.min.x,
                    size.y - (unscale(it->y) - this->bb.min.y),
                    radius
                );
            }
        }
        
        fprintf(f,"\t</g>\n");*/
   // }
   // fprintf(f,"</svg>\n");
//}

std::string
SLMPrint::_SVG_path_d(const Polygon &polygon) const
{
    const Sizef3 size = this->bb.size();
    std::ostringstream d;
    d << "M ";
    for (Points::const_iterator p = polygon.points.begin(); p != polygon.points.end(); ++p) {
        d << unscale(p->x) - this->bb.min.x << " ";
        d << size.y - (unscale(p->y) - this->bb.min.y) << " ";  // mirror Y coordinates as SVG uses downwards Y
    }
    d << "z";
    return d.str();
}

std::string
SLMPrint::_SVG_path_d(const ExPolygon &expolygon) const
{
    std::string pd;
    const Polygons pp = expolygon;
    for (Polygons::const_iterator mp = pp.begin(); mp != pp.end(); ++mp) 
        pd += this->_SVG_path_d(*mp) + " ";
    return pd;
}
	
std::string
SLMPrint::_SVG_path_d(const Polyline &polyline) const
{
    const Sizef3 size = this->bb.size();
    std::ostringstream d;
    d << "M ";
	//for (Polylines::const_iterator pp = polylines.begin(); pp != polylines.end(); ++pp){
    for (Points::const_iterator p = polyline.points.begin(); p != polyline.points.end(); ++p) {
        d << unscale(p->x) - this->bb.min.x << " ";
        d << size.y - (unscale(p->y) - this->bb.min.y) << " ";  // mirror Y coordinates as SVG uses downwards Y
    }
    d << "Z";
    return d.str();
}
	}// name space....



