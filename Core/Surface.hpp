#ifndef slic3r_Surface_hpp_
#define slic3r_Surface_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"


namespace Slic3r {
//- exPolygon  can make many type of surfaces....
enum SurfaceType { stTop, stBottom, stBottomBridge, stInternal, stInternalSolid, stInternalBridge, stInternalVoid,stSupport};
enum BuildStyleID
    {
        stCoreContour_Volume           = 1, /** core contour on volume */
        stCoreContour_Overhang         = 2, /** core contour on powder */
        stHollowShell1Contour_Volume   = 3,/** shell1 contour on volume */
        stHollowShell1Contour_Overhang = 4,/** shell1 contour on powder */
        stHollowShell2Contour_Volume   = 5,/** shell2 contour on volume */
        stHollowShell2Contour_Overhang = 6,/** shell2 contour on powder */
        stCoreOverhangHatch            = 7, /** core hatch on powder */
        stCoreNormalHatch              = 8, /** core hatch on volume */
        stCoreContourHatch             = 9, /** core contour hatch */
        stHollowShell1OverhangHatch    = 10, /** shell1 hatch on powder */
        stHollowShell1NormalHatch      = 11,  /** shell1 hatch on volume */
        stHollowShell1ContourHatch     = 12, /** shell1 contour hatch */
        stHollowShell2OverhangHatch    = 13, /** shell2 hatch on powder */
        stHollowShell2NormalHatch      = 14,  /** shell2 hatch on volume */
        stHollowShell2ContourHatch     = 15, /** shell2 contour hatch */
        stSupportContourVolume         = 16, /** support contour */
        stSupportHatch                 = 17,  /** support hatch */
        stPointSequence                = 18, /** point sequence */
        stExternalSupports             = 19, /** Externe Stützen */
        stCoreContourHatchOverhang     = 20, /** HollowCore Konturversatz - Overhang */
        stHollowShell1ContourHatchOverhang = 21, /** HollowShell1  - Overhang */
        stHollowShell2ContourHatchOverhang = 22, /** HollowShell2 Konturversatz - Overhang */
    };
class Surface
{
    public:
    BuildStyleID    bstyle_id;
    SurfaceType     surface_type;
    ExPolygon       expolygon;
    double          thickness;          // in mm
    unsigned short  thickness_layers;   // in layers
    double          bridge_angle;       // in radians, ccw, 0 = East, only 0+ (negative means undefined)
    unsigned short  extra_perimeters;
    
    Surface(SurfaceType _surface_type, const ExPolygon &_expolygon)
        : surface_type(_surface_type), expolygon(_expolygon),
            thickness(-1), thickness_layers(1), bridge_angle(-1), extra_perimeters(0)
        {};
    operator Polygons() const;
    double area() const;
    bool is_solid() const;
    bool is_external() const;
    bool is_internal() const;
    bool is_bottom() const;
    bool is_support() const;
	bool is_bridge() const;
	bool is_overhang() const;
	bool is_over_overhang() const;
	bool is_core() const;
	bool is_top() const;
	bool contains(const Point &point) const;
    ExPolygons RectGrids(float island_L,float island_H)const;
};

typedef std::vector<Surface> Surfaces;
typedef std::vector<Surface*> SurfacesPtr;
typedef std::vector<const Surface*> SurfacesConstPtr;

inline Polygons
to_polygons(const Surfaces &surfaces)
{
    Slic3r::Polygons pp;
    for (Surfaces::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)*s);
    return pp;
}

inline Polygons
to_polygons(const SurfacesPtr &surfaces)
{
    Slic3r::Polygons pp;
    for (SurfacesPtr::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)**s);
    return pp;
}

inline Polygons
to_polygons(const SurfacesConstPtr &surfaces)
{
    Slic3r::Polygons pp;
    for (SurfacesConstPtr::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)**s);
    return pp;
}

inline ExPolygons to_expolygons(const Surfaces &src)
{
    ExPolygons expolygons;
    expolygons.reserve(src.size());
    for (Surfaces::const_iterator it = src.begin(); it != src.end(); ++it)
        expolygons.push_back(it->expolygon);
    return expolygons;
}

inline ExPolygons to_expolygons(Surfaces &&src)
{
	ExPolygons expolygons;
	expolygons.reserve(src.size());
	for (Surfaces::const_iterator it = src.begin(); it != src.end(); ++it)
		expolygons.emplace_back(ExPolygon(std::move(it->expolygon)));
	src.clear();
	return expolygons;
}

inline ExPolygons to_expolygons(const SurfacesPtr &src)
{
    ExPolygons expolygons;
    expolygons.reserve(src.size());
    for (SurfacesPtr::const_iterator it = src.begin(); it != src.end(); ++it)
        expolygons.push_back((*it)->expolygon);
    return expolygons;
}


// -Count a nuber of polygons stored inside the vector of expolygons.
// -Useful for allocating space for polygons when converting expolygons to polygons.
inline size_t number_polygons(const Surfaces &surfaces)
{
    size_t n_polygons = 0;
    for (Surfaces::const_iterator it = surfaces.begin(); it != surfaces.end(); ++ it)
        n_polygons += it->expolygon.holes.size() + 1;
    return n_polygons;
}

inline size_t number_polygons(const SurfacesPtr &surfaces)
{
    size_t n_polygons = 0;
    for (SurfacesPtr::const_iterator it = surfaces.begin(); it != surfaces.end(); ++ it)
        n_polygons += (*it)->expolygon.holes.size() + 1;
    return n_polygons;
}

// Append a vector of Surfaces at the end of another vector of polygons.
inline void polygons_append(Polygons &dst, const Surfaces &src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (Surfaces::const_iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back(it->expolygon.contour);
        dst.insert(dst.end(), it->expolygon.holes.begin(), it->expolygon.holes.end());
    }
}

inline void polygons_append(Polygons &dst, Surfaces &&src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (Surfaces::iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back(std::move(it->expolygon.contour));
        std::move(std::begin(it->expolygon.holes), std::end(it->expolygon.holes), std::back_inserter(dst));
        it->expolygon.holes.clear();
    }
}

// Append a vector of Surfaces at the end of another vector of polygons.
inline void polygons_append(Polygons &dst, const SurfacesPtr &src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (SurfacesPtr::const_iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back((*it)->expolygon.contour);
        dst.insert(dst.end(), (*it)->expolygon.holes.begin(), (*it)->expolygon.holes.end());
    }
}

inline void polygons_append(Polygons &dst, SurfacesPtr &&src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (SurfacesPtr::const_iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back(std::move((*it)->expolygon.contour));
        std::move(std::begin((*it)->expolygon.holes), std::end((*it)->expolygon.holes), std::back_inserter(dst));
        (*it)->expolygon.holes.clear();
    }
}


}

#endif
