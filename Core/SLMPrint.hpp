#ifndef slic3r_SLMPrint_hpp_
#define slic3r_SLMPrint_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"
#include "ExPolygonCollection.hpp"
#include "Fill/Fill.hpp"
#include "Model.hpp"
#include "Point.hpp"
#include "PrintConfig.hpp"
#include "SVG.hpp"
#include "Polyline.hpp"
#include "MultiPoint.hpp"
//#include "GCode.hpp"
#include "Line.hpp"
#include <vector>
#include <iostream>
#include <fstream>
namespace Slic3r {
	


class SLMPrint
{
    public:
    SLMPrintConfig config;
    FullPrintConfig configfull;
    class Layer {
        public:
        //SLMPrint(Model *m){model=m;}
        
    ExPolygonCollection slices;   // just the  vector of expolygons in a layer..
    std::vector<Polygons> perimeters;
	std::vector<Polygons>  support;
	ExPolygonCollection offset_Slices;
    ExtrusionEntityCollection infill;
    ExPolygonCollection solid_infill; 
	std::vector<Polylines> LaserVectors;//
	ExPolygonCollection mGrid;//
	std::vector<Lines> mGridLines;//
	std::vector<ExPolygons> gridpolygons;
	Polylines supportpillers;
    float slice_z, print_z;
    bool solid;
    Layer(float _slice_z, float _print_z)
            : slice_z(_slice_z), print_z(_print_z), solid(true) {};
    };
    std::vector<Layer> layers;
    
    class SupportPillar : public Point {
        public:
        size_t top_layer, bottom_layer;
        SupportPillar(const Point &p) : Point(p), top_layer(0), bottom_layer(0) {};
    };
    std::vector<SupportPillar> sm_pillars;
    void setModel(Model *m){model=m;}
    void slice();
    void write_Polygons(const std::string &outputfile) const;
    void write_vectors(const std::string &outputfile) const;
    void write_svg(const std::string &outputfile) const;
    void write_grid(const std::string &outputfile) const;
    void MakeGrids();
    void Export_SLM_GCode(const std::string &outputfile) const;

   private:
    Model* model;
    BoundingBoxf3 bb;
    Polygon  DMLSClipperPath_to_Slic3rPolygon(const ClipperLib::Path &input);
    Polygons DMLSClipperPaths_to_Slic3rPolygons(const ClipperLib::Paths &input);
    //Polygon gridShift(coord_t X_shift,coord_t Y_shift);
    void _infill_layer(size_t i, const Fill* fill);
    coordf_t sm_pillars_radius() const;
    std::string _SVG_path_d(const Polygon &polygon) const;
    std::string _SVG_path_d(const ExPolygon &expolygon) const;
    std::string _SVG_path_d(const Polyline &polyline) const;
    std::string polygon_d(const Polygon &polygon) const;
    std::string polygon_d(const ExPolygon &expolygon) const;
    std::string polygon_d(const Polyline &polyline) const;

    void BoundRect();
    void RectGrids();
	ClipperLib::Paths solution;
	Polygons rectangularGrids;
	Point Rmin;
    Point  Rmax;
	Polygon grid;
	Polygon Conto;
	Polygons Holes;
	Points gridp;
    void Make_Support() ;
    
    
};

}

#endif
