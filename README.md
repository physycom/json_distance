---
documentclass: physycomen
title:  "json_distance"
author: "Fabbri, Sinigardi"
---

<a href="http://www.physycom.unibo.it"> 
<div class="image">
<img src="https://cdn.rawgit.com/physycom/templates/697b327d/logo_unibo.png" width="90" height="90" alt="© Physics of Complex Systems Laboratory - Physics and Astronomy Department - University of Bologna"> 
</div>
</a>
<a href="https://travis-ci.org/physycom/json_distance"> 
<div class="image">
<img src="https://travis-ci.org/physycom/json_distance.png?branch=master" width="90" height="20" alt="Build Status"> 
</div>
</a>
<a href="https://ci.appveyor.com/project/cenit/json-distance"> 
<div class="image">
<img src="https://ci.appveyor.com/api/projects/status/ea04bvtigk4axh0q?svg=true" width="90" height="20" alt="Build Status"> 
</div>
</a>


### Purpose
This tool has been written in order to benchmark GNSS solution performances with respect to a reference system. It takes two .json files as input, one for the reference device and one for the benchmarked device, it interpolates the reference at the exact timestamp of each benchmarked device measurement and it gives the distance between the two points.


### Installation
**CMake** and a **C++11** compatible compiler are required. To build the executable, clone the repo and then type  
```
mkdir build ; cd build ; cmake .. ; cmake --build . --target install
```
With CMake you can also deploy projects for the most common IDEs.  
Uses [jsoncons](https://github.com/danielaparker/jsoncons) as a git submodule.


### Usage
```
json_distance.exe -i input1.json input2.json -o output.json
```
where *input1.json* and *input2.json* must be existing and valid .json files, while *output.json* is the name of the output .json with the distance between selected points in the input data.

More details about file formats is available [here](https://github.com/physycom/file_format_specifications/blob/master/formati_file.md).
