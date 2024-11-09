#include "../ClipperUtils.hpp"
#include "../ExPolygon.hpp"
#include "../Flow.hpp"
#include "../PolylineCollection.hpp"
#include "../Surface.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "FillDMLSisland.hpp"

namespace Slic3r {



void 
DMLSGrids::BoundRect(ClipperLib::Path Cont)
{
    Rmin.X=9999999000;
    Rmin.Y=9999999000;
    Rmax.X=-9999999000;
    Rmax.Y=-9999999000;
         //foreach(ClipperLib::IntPoint pts, Cont)
		 for (ClipperLib::Path::const_iterator pts = Cont.begin(); pts !=Cont.end(); ++pts)
        {
            if(pts->X <  Rmin.X)
                 Rmin.X=pts->X;
            if(pts->Y < Rmin.Y)
                Rmin.Y=pts->Y;

            if(pts->X > Rmax.X)
                   Rmax.X=pts->X;
            if(pts->Y > Rmax.Y)
                 Rmax.Y=pts->Y;
        }
         return;
}//