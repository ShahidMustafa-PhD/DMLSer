

### DMLSer?

DMLSer is mainly a **Slice File** generator for Direct Metal Laser Sintering (DMLS)  3D printers: it reads 3D models (STL, OBJ, AMF, 3MF) and it converts them into **Slice File** containg instructions fo DMLS  machine.
 It can serve as a platform for implementing several **new (experimental) ideas**, such as multiple laser sources, laser parameters development,  variable-height layers, per-object customized settings, support modifiers,  and more.  It is not yet complete but under active develoment.

DMLSer is:

* **Open:** it is totally **open source** and it's **independent from any commercial company** or printer manufacturer.
* **Compatible:** it supports only Common Layer Interface  file 
* **Advanced:** many configuration options allow for fine-tuning and full control.
* **Robust:** the codebase includes libslic3r which had been under regression tests, collected in 6 years of development.
* **Modular:** the core of DMLSer is libslic3r, a C++ library that provides a granular API and reusable components.

### <a name="features"></a>Features

(Most of these are also available in the command line interface.)

* **CLI file** for DMLS/SLM printers;
* **conversion** between STL, OBJ, AMF, 3MF and POV formats;
* **auto-repair** of non-manifold meshes (and ability to re-export them);
* **SVG export** of slices;
* tool for **cutting meshes** in multiple solid parts with visual preview (also in batch using a grid);.

### What language is it written in?

The core parts of DMLSer are written in C++14, with multithreading. The graphical interface is in the process of being developed.

### How to install?
ToDo

### Can I help?

ToDo

### Directory structure

* `dmlslicer/`: the vcpkg scripts used for dependencies and  the CMake definition file for compiling it
* `src/`: the C++ source of the `DMLSer` source files.
* `t/`: the test suite to be included


### Acknowledgements

The main author of DMLSer is Dr. Shahid Mustafa 


### How can I invoke DMLSer using the command line?

DMLSer --export-svg  model1.stl  model2.stl  --load model1.ini   --load model2.ini 
 
