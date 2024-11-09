#ifndef Slic3r_SLMSlicer_hpp
#define Slic3r_SLMSlicer_hpp
#include <boost/nowide/args.hpp>
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
 namespace Slic3r
 {
class SLMSlicer
{
   public:
   int argc;
   char **argv;
   SLMSlicer(int ar,char **arv); //: argc(ar), argv(arg) {};
   ~SLMSlicer();
   bool print();   	
};

 }



#endif
