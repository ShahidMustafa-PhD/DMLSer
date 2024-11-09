	
#define NDEBUG 

#include "Config.hpp"
#include "Geometry.hpp"
#include "IO.hpp"
#include "Model.hpp"
#include "Print.hpp"
#include "SLMPrint.hpp"
#include "TriangleMesh.hpp"
#include "libslic3r.h"
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>

using namespace Slic3r;

void confess_at(const char *file, int line, const char *func, const char *pat, ...){}
DynamicPrintConfig  volumeconfig(std::string file);
int
main(int argc, char **argv)
{    
    
    // Convert arguments to UTF-8 (needed on Windows).
    // argv then points to memory owned by a.
    // boost::nowide::args a(argc, argv);
    
    // -parse all command line options into a DynamicConfig
    ConfigDef config_def; // always neede to declare
    config_def.merge(cli_config_def);  // global declaration  of class object..
    config_def.merge(print_config_def); //  global declaration of class object 
    // -the global configuration objects are  merged.
    DynamicConfig config(&config_def); //  Just an object of dynamic config
    t_config_option_keys input_files; //  just the  string vector
    config.read_cli(argc, argv, &input_files);//reads the CLI commands 
    // -saves the name of config.ini files(--load  [name1].ini --load  [name2].ini)
    // -saves the names of *.stl files in  vector input_files 
    // -apply command line options to a more handy CLIConfig
    CLIConfig cli_config;
    cli_config.apply(config, true); // -seperates the cli commands 
    DynamicPrintConfig print_config;  //-every volume meeds this 

    // load config files supplied via --load
    for (const std::string &file : cli_config.load.values) 
    {
        
		if (!boost::filesystem::exists(file))
         {
            boost::nowide::cout << "No such file: " << file << std::endl;
            exit(1);
         }
	
        DynamicPrintConfig c;
		//-here it reads the given .ini file and loads all the values..from file.
        try {
            c.load(file);  // -loads if *.ini file. note *.ini is value of --load command
            } catch (std::exception &e) 
            {
            boost::nowide::cerr << "Error while reading config file: " << e.what() << std::endl;
            exit(1);
            }
        c.normalize();
        print_config.apply(c);
    }
    // -print_config has been updated  now..
    // -apply command line options to a more specific DynamicPrintConfig which provides normalize()
    // -(command line options override --load files)
    
    	 
    // -write config if requested
    if (!cli_config.save.value.empty()) print_config.save(cli_config.save.value);
    // -boost::nowide::cerr << "No such file: " << cli_config.save.value<< std::endl;
    // -read input file(s) if any
    // -std::vector<Model> models;
	Model  slm_model;
	Print* slm_print =new Print();
	ModelObject* modobj=slm_model.add_object();
	slm_model.repair();
        ModelVolume* stlvolumeptr;
	// -for every stl
        //- input_files  contains all the stl files....
	for (const t_config_option_key &file : input_files) {
            if (!boost::filesystem::exists(file)) 
            boost::nowide::cerr << "No such file: " << file << std::endl;
            else
            {		    
	    TriangleMesh modmesh; 
	    std::string name= file.substr(0, file.size()-4);
        try {
                modmesh.ReadSTLFile(file);	
		std::string name= file.substr(0, file.size()-4);
		stlvolumeptr=modobj->add_volume(modmesh);
		stlvolumeptr->name=name;
		stlvolumeptr->modifier=false;
				
        } catch (std::exception &e) {
			
            boost::nowide::cerr << file << ": " << e.what() << std::endl;
            exit(1);
        }
     // -now   load config files supplied via --load
	 	
    for (const std::string &config_file : cli_config.load.values) 
        {
                std::string name_config= config_file.substr(0, config_file.size()-4);
		if(name_config.compare(name) == 0)
		{  
	        DynamicPrintConfig print_config;
	        DynamicPrintConfig c =volumeconfig(config_file);
	        print_config.apply(c);
		print_config.apply(config, true);
                print_config.normalize();
		stlvolumeptr->config=print_config;
		slm_print->apply_config(print_config);
	        boost::nowide::cout << name<<  " :=: "<<name_config <<std::endl;
			break;
		}
	    }
	}  //-else
}  //-for input files...
 
		
if (slm_model.objects.empty()) 
{
    boost::nowide::cout << "empty: " << std::endl;
    boost::nowide::cerr << "Error: Model object empty: "  << std::endl;
    //continue;
    exit(1);
}
slm_model.add_default_instances();

modobj->arrange_volumes();
modobj->update_bounding_box();
modobj->center_around_origin();		
slm_print->add_model_object(modobj);
boost::nowide::cout << "...size of objects to..." << slm_model.objects.size() << std::endl;
//for (Model &slm_model : models) {
boost::nowide::cout << "......Welcome to DMLS....."  << std::endl;
if (cli_config.info) 
{
// --info works on unrepaired slm_model
slm_model.print_info();
} 
else if (cli_config.export_svg) 
           {
         	std::cout  <<".....svg now:...... " << std::endl;
		slm_print->objects.front()->_slice(); 
		boost::nowide::cout << ".......slice done........: " << std::endl;
		slm_print->objects.front()->detect_surfaces_type();//slm_model.bounding_box()
                slm_print->objects.front()->_infill();//slm_model.bounding_box()
		slm_print->objects.front()->Support_pillers();
		slm_print->objects.front()->write_svg();

        }  else {
                boost::nowide::cerr << "error: command not supported" << std::endl;
            return 1;
        }
    
    
    return 0;
}

DynamicPrintConfig  volumeconfig(std::string file)
{    	
if (!boost::filesystem::exists(file)) {
            boost::nowide::cout << "No such file: " << file << std::endl;
            exit(1);
        }
	 DynamicPrintConfig c;
		// Here it reads the given .ini file and loads all the values..from file.
        try {
            c.load(file);  //  loads if *.ini file. note *.ini is value 0f --load command
        } catch (std::exception &e) {
            boost::nowide::cout << "Error while reading config file: " << e.what() << std::endl;
            exit(1);
        }
        c.normalize();
        //print_config.apply(c);
    return c;

}
