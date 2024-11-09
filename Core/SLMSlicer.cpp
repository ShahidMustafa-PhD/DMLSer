	
#ifndef slic3r_SLMSlicer_hpp_
#define slic3r_SLMSlicer_hpp_

#include "Config.hpp"
#include "Geometry.hpp"
#include "IO.hpp"
#include "Model.hpp"
#include "Print.hpp"
//#include "SLAPrint.hpp"
#include "SLMPrint.hpp"
#include "TriangleMesh.hpp"
#include "libslic3r.h"
#include "SLMSlicer.hpp"
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>

 namespace Slic3r
{

void confess_at(const char *file, int line, const char *func, const char *pat, ...){}

//SLMSlicer::SLMSlicer(int argc, char** argv)
//DynamicConfig::read_cli(int argc, char** argv, t_config_option_keys* extra)
SLMSlicer::SLMSlicer(int ar,char **arv)
: argc(ar),argv(arv) {};

SLMSlicer::~SLMSlicer()
{
};

bool
SLMSlicer::print()
{       
    // -Convert arguments to UTF-8 (needed on Windows).
    // -argv then points to memory owned by a.
    boost::nowide::args a(this->argc, this->argv);
    // -parse all command line options into a DynamicConfig
    ConfigDef config_def; // always neede to declare
    config_def.merge(cli_config_def);  // global declaration  of class object..
    config_def.merge(print_config_def); //  global declaration of class object 
    // -the global configuration objects are  merged.
	DynamicConfig config(&config_def); //  Just an object of dynamic config
    t_config_option_keys input_files; //  just the  string vector
	 std::cout << "print2"<< std::endl;
	config.read_cli(this->argc, this->argv, &input_files);//reads the CLI commands 
	
	// -saves the name of config.ini files  in cli_config.load.values (--load  [name2].ini --load  [name2].ini)
    // -saves the names of *.stl files in  vector input_files 
    // -apply command line options to a more handy CLIConfig
    CLIConfig cli_config;
    cli_config.apply(config, true); // -seperates the cli commands 
    
    DynamicPrintConfig print_config;  // -every volume meeds this 

    // -load config files supplied via --load
    for (const std::string &file : cli_config.load.values) 
    {
        std::cout <<" ini files  "<< file<< std::endl;
		if (!boost::filesystem::exists(file)) 
            {
            boost::nowide::cout << "No such file: " << file << std::endl;
            exit(1);
            }
		
	
    //-boost::nowide::cout << "No such file: " << cli_config.load.values << std::endl;
        DynamicPrintConfig c;
	//-here it reads the given .ini file and loads all the values..from file.
        try 
        {
            c.load(file);  //  loads if *.ini file. note *.ini is value 0f --load command
        } catch (std::exception &e) 
            {
            boost::nowide::cerr << "Error while reading config file: " << e.what() << std::endl;
            exit(1);
            }
        c.normalize();
        print_config.apply(c);
    }
	 std::cout <<" inis read finished success...!  "<< std::endl;
    // -print_config has been updated  now..
    // -apply command line options to a more specific DynamicPrintConfig which provides normalize()
    // -(command line options override --load files)
    
    if (!cli_config.save.value.empty()) print_config.save(cli_config.save.value);
	Model  slm_model;
    Print* slm_print =new Print();
	//SLMPrint* slm_print =new SLMPrint();
	//std::vector<ModelVolume*> stlvolumeptrs;
	ModelObject* modobj=slm_model.add_object();
	//std::vector<std::string> stls;
	slm_model.repair();
    ModelVolume* stlvolumeptr;
	// for every stl
 //  read the stl files and assign configurations to models...
	for (const t_config_option_key &file : input_files)
       {
         std::cout <<" stl files   "<<file<< std::endl;
		if (!boost::filesystem::exists(file)) 
		{ boost::nowide::cerr << "No such file: " << file << std::endl;
		exit(1);
        }
      else
{		   
		//DynamicPrintConfig print_config;  //  Every volume meeds this 
	    TriangleMesh modmesh; 
		std::cout <<" mesh now...  "<< std::endl;
		std::string name= file.substr(0, file.size()-4);
        try {
			modmesh.repair();
            modmesh.ReadSTLFile(file);
			std::string name= file.substr(0, file.size()-4);
		    stlvolumeptr=modobj->add_volume(modmesh);
			stlvolumeptr->name=name;
			stlvolumeptr->modifier=false;  // its not the modifier
        } catch (std::exception &e) {
			 std::cout <<" exception  "<< std::endl;
            boost::nowide::cerr << file << ": " << e.what() << std::endl;
            exit(1);
        }
     // load config files supplied via --load  	
    for (const std::string &config_file : cli_config.load.values) {
        std::string name_config= config_file.substr(0, config_file.size()-4);
		if(name_config.compare(name) == 0)
		{  DynamicPrintConfig print_config;
		   DynamicPrintConfig c ;//=this->volumeconfig(config_file);
//  start extracting     configs

    {    	
if (!boost::filesystem::exists(config_file)) {
            boost::nowide::cout << "No such file: " << file << std::endl;
            exit(1);
        }
	
		// Here it reads the given .ini file and loads all the values..from file.
        try {
            c.load(config_file);  //  loads if *.ini file. note *.ini is value 0f --load command
        } catch (std::exception &e) {
            boost::nowide::cout << "Error while reading config file: " << e.what() << std::endl;
            exit(1);
        }
     }  
//-configs  extracted    now  its time to  apply...		 
			c.normalize();

	        print_config.apply(c);
			print_config.apply(config, true);
            print_config.normalize();
			stlvolumeptr->config=print_config;
			slm_print->apply_config(print_config);
		    boost::nowide::cout << name<<  " :=: "<<name_config <<std::endl;
			break;
		}
			}
}  //else
}  // for input files...
 
		
        if (slm_model.objects.empty()) 
        {
			boost::nowide::cout << "empty: " << std::endl;
            boost::nowide::cerr << "Error: Model object empty: "  << std::endl;
            //continue;
			exit(1);
        }
 
        // apply command line transform options
        /*for (ModelObject* o : slm_model.objects) {
            if (cli_config.scale_to_fit.is_positive_volume())
            o->scale_to_fit(cli_config.scale_to_fit.value);
            // TODO: honor option order?
            o->scale(cli_config.scale.value);
            o->rotate(Geometry::deg2rad(cli_config.rotate_x.value), X);
            o->rotate(Geometry::deg2rad(cli_config.rotate_y.value), Y);
            o->rotate(Geometry::deg2rad(cli_config.rotate.value), Z);
        }*/
       // models.push_back(slm_model);
		//slm_model.merge(slm_model)
		slm_model.add_default_instances();
		boost::nowide::cout << "pX: " << std::endl;
       //modobj->align_to_ground();
		modobj->arrange_volumes();
        modobj->update_bounding_box();
        modobj->center_around_origin();		
		slm_print->add_model_object(modobj);
		boost::nowide::cout << "pos1: " << std::endl;
	    //slm_print->apply_config(print_config);
		boost::nowide::cout << "pos1: " << std::endl;
		
	//slm_print->add_model_object(modobj);
	//slm_print.apply_config(print_config);
	boost::nowide::cout << "size of objects to " << slm_model.objects.size() << std::endl;
    //for (Model &slm_model : models) {
	boost::nowide::cout <<"Wellcome  to DMLS";
        if (cli_config.info) {
            // --info works on unrepaired slm_model
            slm_model.print_info();
        } else if (cli_config.export_obj) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = slm_model.objects.front()->input_file + ".obj";
    
            TriangleMesh mesh = slm_model.mesh();
            mesh.repair();
            IO::OBJ::write(mesh, outfile);
            boost::nowide::cout << "File exported to " << outfile << std::endl;
        } else if (cli_config.export_pov) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = slm_model.objects.front()->input_file + ".pov";
    
            TriangleMesh mesh = slm_model.mesh();
            mesh.repair();
            IO::POV::write(mesh, outfile);
            boost::nowide::cout << "File exported to " << outfile << std::endl;
        } else if (cli_config.export_svg) {
            /*std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = slm_model.objects.front()->input_file + ".txt";
            if (outfile.empty())  std::string outfile2 = slm_model.objects.front()->input_file+"Gcode" + ".txt";
            SLMPrint print(&slm_model);
            print.config.apply(print_config, true);
			print.configfull.apply(print_config, true);
            print.slice();
            print.write_Polygons(outfile);
			//print.write_vectors(std::string("vecs.txt"));
			print.Export_SLM_GCode(std::string("gcode.txt"));
			//print.write_grid(outfile);
			 print.write_svg(std::string("Layer"));
		
            boost::nowide::cout << "SVG file exported to " << outfile << std::endl;*/
			//-------------------------------------------------//
			// Donot follow Print method. ıt is specific to Extruders...
			//Print  myprint;
			//myprint.apply_config(print_config);
			//myprint.add_model_object(slm_model.objects.front());
			boost::nowide::cout << "svg now: " << std::endl;
			slm_print->objects.front()->_slice();
		       //while(!slm_print->objects.front()->state.is_done(posSlice));
			   
			boost::nowide::cout << "slice done: " << std::endl;
			slm_print->objects.front()->detect_surfaces_type();//slm_model.bounding_box()
            slm_print->objects.front()->_infill();//slm_model.bounding_box()
			 // while(!slm_print->objects.front()->state.is_done(posInfill));
			slm_print->objects.front()->Support_pillers();
			slm_print->objects.front()->write_svg();
			   //boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
			 //slm_print->print_svg(std::string("printsvg"));
        } else if (cli_config.export_3mf) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = slm_model.objects.front()->input_file;
            // Check if the file is already a 3mf.
            if(outfile.substr(outfile.find_last_of('.'), outfile.length()) == ".3mf")
                outfile = outfile.substr(0, outfile.find_last_of('.')) + "_2" + ".3mf";
            else
                // Remove the previous extension and add .3mf extention.
            outfile = outfile.substr(0, outfile.find_last_of('.')) + ".3mf";
            IO::TMF::write(slm_model, outfile);
            boost::nowide::cout << "File file exported to " << outfile << std::endl;
        } else if (cli_config.cut_x > 0 || cli_config.cut_y > 0 || cli_config.cut > 0) {
            slm_model.repair();
            slm_model.translate(0, 0, -slm_model.bounding_box().min.z);
            
            if (!slm_model.objects.empty()) {
                // FIXME: cut all objects
                Model out;
                if (cli_config.cut_x > 0) {
                    slm_model.objects.front()->cut(X, cli_config.cut_x, &out);
                } else if (cli_config.cut_y > 0) {
                    slm_model.objects.front()->cut(Y, cli_config.cut_y, &out);
                } else {
                    slm_model.objects.front()->cut(Z, cli_config.cut, &out);
                }
                
                ModelObject &upper = *out.objects[0];
                ModelObject &lower = *out.objects[1];

                // Use the input name and trim off the extension.
                std::string outfile = cli_config.output.value;
                if (outfile.empty()) outfile = slm_model.objects.front()->input_file;
                outfile = outfile.substr(0, outfile.find_last_of('.'));
                std::cerr << outfile << "\n";
            
                if (upper.facets_count() > 0) {
                    TriangleMesh m = upper.mesh();
                    IO::STL::write(m, outfile + "_upper.stl");
                }
                if (lower.facets_count() > 0) {
                    TriangleMesh m = lower.mesh();
                    IO::STL::write(m, outfile + "_lower.stl");
                }
            }
        } else if (cli_config.cut_grid.value.x > 0 && cli_config.cut_grid.value.y > 0) {
            TriangleMesh mesh = slm_model.mesh();
            mesh.repair();
            
            TriangleMeshPtrs meshes = mesh.cut_by_grid(cli_config.cut_grid.value);
            size_t i = 0;
            for (TriangleMesh* m : meshes) {
                std::ostringstream ss;
                ss << slm_model.objects.front()->input_file << "_" << i++ << ".stl";
                IO::STL::write(*m, ss.str());
                delete m;
            }
        } else {
            boost::nowide::cerr << "error: command not supported" << std::endl;
            return 1;
        }
    
    
    return 0;
};


}


#endif