/* Copyright 2015 - Alessandro Fabbri, Stefano Sinigardi */

/***************************************************************************
This file is part of json_distance.

json_distance is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

json_distance is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with json_distance. If not, see <http://www.gnu.org/licenses/>.
***************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <stdexcept>
#include <string>
#include <cmath>
#include <map>

#include "jsoncons/json.hpp"

#define GEODESIC_DEG_TO_M         111070.4                              // conversion [deg] -> [meter]
#define RAD_TO_DEG                57.2957795131                         // 180/pi
#define DEG_TO_RAD                1.745329251e-2                        // pi/180
#define EPSILON                   1e-5

#define MAJOR_VERSION             1
#define MINOR_VERSION             1

double mapping(double x, double old_min, double old_max, double new_min, double new_max) {
  return (x - old_min) / (old_max - old_min)*(new_max - new_min) + new_min;
};

void usage(char* progname) {
  // Usage
  std::cout << std::endl << "\tUsage:\t" << progname << " -i [input1.json] -d [input2.json] -o [output.json] [-a]" << std::endl << std::endl;
  std::cout << "We distinguish between -i and -d because the program is going to calculate distance between points\n"
    << "in [input1.json] from a virtual point in [input2.json] at the same identical timestamp,\n"
    << "calculated interpolating points inside it. An optional -a is used to not filter points in (0,0)" << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::cout << "JSON DISTANCE Calculator v" << MAJOR_VERSION << "." << MINOR_VERSION << std::endl;

  std::string input1_name, input2_name, output_name;
  bool all = false;

  if (argc > 2) { /* Parse arguments, if there are arguments supplied */
    for (int i = 1; i < argc; i++) {
      if ((argv[i][0] == '-') || (argv[i][0] == '/')) {       // switches or options...
        switch (tolower(argv[i][1])) {
        case 'i':
          input1_name = argv[++i];
          break;
        case 'a':
          all = true;
          break;
        case 'd':
          input2_name = argv[++i];
          break;
        case 'o':
          output_name = argv[++i];
          break;
        default:    // no match...
          std::cout << "ERROR: Flag \"" << argv[i] << "\" not recognized. Quitting..." << std::endl;
          usage(argv[0]);
        }
      }
      else {
        std::cout << "ERROR: Flag \"" << argv[i] << "\" not recognized. Quitting..." << std::endl;
        usage(argv[0]);
      }
    }
  }
  else {
    std::cout << "ERROR: No flags specified. Read usage and relaunch properly." << std::endl;
    usage(argv[0]);
  }

  //Safety checks for file manipulations
  std::ofstream output_file;
  std::ifstream input_file;

  if (input1_name.size() > 5) {
    if (input1_name.substr(input1_name.size() - 5, 5) != ".json") {
      std::cout << input1_name << " is not a valid .json file. Quitting..." << std::endl;
      exit(2);
    }
  }
  else {
    std::cout << input1_name << " is not a valid .json file. Quitting..." << std::endl;
    exit(22);
  }
  input_file.open(input1_name.c_str());
  if (!input_file.is_open()) {
    std::cout << "FAILED: Input file " << input1_name << " could not be opened. Quitting..." << std::endl;
    exit(222);
  }
  else { std::cout << "SUCCESS: file " << input1_name << " opened!\n"; }
  input_file.close();

  if (input2_name.size() > 5) {
    if (input2_name.substr(input2_name.size() - 5, 5) != ".json") {
      std::cout << input2_name << " is not a valid .json file. Quitting..." << std::endl;
      exit(2);
    }
  }
  else {
    std::cout << input2_name << " is not a valid .json file. Quitting..." << std::endl;
    exit(22);
  }
  input_file.open(input2_name.c_str());
  if (!input_file.is_open()) {
    std::cout << "FAILED: Input file " << input2_name << " could not be opened. Quitting..." << std::endl;
    exit(222);
  }
  else { std::cout << "SUCCESS: file " << input2_name << " opened!\n"; }
  input_file.close();


  if (output_name.size() > 5) {
    if (output_name.substr(output_name.size() - 5, 5) != ".json") {
      std::cout << output_name << " is not a valid .json file. Quitting..." << std::endl;
      exit(3);
    }
  }
  else {
    std::cout << output_name << " is not a valid .json file. Quitting..." << std::endl;
    exit(33);
  }

  output_file.open(output_name.c_str());
  if (!output_file.is_open()) {
    std::cout << "FAILED: Output file " << output_name << " could not be opened. Quitting..." << std::endl;
    exit(333);
  }
  else { std::cout << "SUCCESS: file " << output_name << " opened!" << std::endl; }


  // Import JSON gps databases
  jsoncons::json gps_records_1 = jsoncons::json::parse_file(input1_name);
  jsoncons::json gps_records_2 = jsoncons::json::parse_file(input2_name);

  std::vector<double> lat1, lon1, t1, lat2, lon2, t2;
  if (gps_records_1.is_array()) {
    for (size_t i = 0; i < gps_records_1.size(); i++) {
      lat1.push_back(gps_records_1[i].has_member("lat") ? gps_records_1[i]["lat"].as<double>() : 90.0);
      lon1.push_back(gps_records_1[i].has_member("lon") ? gps_records_1[i]["lon"].as<double>() : 90.0);
      t1.push_back(gps_records_1[i].has_member("timestamp") ? gps_records_1[i]["timestamp"].as<double>() : 90.0);
    }
  }
  else if (gps_records_1.is_object()) {
    for (auto it = gps_records_1.begin_members(); it != gps_records_1.end_members(); it++) {
      lat1.push_back(it->value().has_member("lat") ? it->value()["lat"].as<double>() : 90.0);
      lon1.push_back(it->value().has_member("lon") ? it->value()["lon"].as<double>() : 90.0);
      t1.push_back(it->value().has_member("timestamp") ? it->value()["timestamp"].as<double>() : 0.0);
    }
  }

  if (gps_records_2.is_array()) {
    for (size_t i = 0; i < gps_records_2.size(); i++) {
      lat2.push_back(gps_records_2[i].has_member("lat") ? gps_records_2[i]["lat"].as<double>() : 90.0);
      lon2.push_back(gps_records_2[i].has_member("lon") ? gps_records_2[i]["lon"].as<double>() : 90.0);
      t2.push_back(gps_records_2[i].has_member("timestamp") ? gps_records_2[i]["timestamp"].as<double>() : 0.0);
    }
  }
  else if (gps_records_2.is_object()) {
    for (auto it = gps_records_2.begin_members(); it != gps_records_2.end_members(); it++) {
      lat2.push_back(it->value().has_member("lat") ? it->value()["lat"].as<double>() : 90.0);
      lon2.push_back(it->value().has_member("lon") ? it->value()["lon"].as<double>() : 90.0);
      t2.push_back(it->value().has_member("timestamp") ? it->value()["timestamp"].as<double>() : 0.0);
    }
  }


  std::map<int, std::pair<int, int> > time_map;
  for (size_t i = 0; i < t1.size(); i++) {
    int prev = -1, next = -1;
    for (size_t j = 0; j < t2.size(); j++) {
      if (t1[i] < t2[j] && prev == -1) prev = j - 1;
      if (t1[i] > t2[t2.size() - 1 - j] && next == -1) next = t2.size() - j;
    }
    if (prev == -1 || next == -1) continue;
    time_map[i] = std::make_pair(prev, next);
  }

  std::cout << "Input size       : " << std::setw(6) << lat1.size() << "  " << std::setw(6) << lon1.size() << "  " << std::setw(6) << t1.size() << std::endl;
  std::cout << "Reference size   : " << std::setw(6) << lat2.size() << "  " << std::setw(6) << lon2.size() << "  " << std::setw(6) << t2.size() << std::endl;
  std::cout << "Connected points : " << std::setw(6) << time_map.size() << std::endl;

  jsoncons::json gps_records_distance(jsoncons::json::an_array);
  size_t counter = 1;

  for (auto it = time_map.begin(); it != time_map.end(); it++, counter++) {
    jsoncons::json input_gnss_coordinates, bounding_coordinates;
    double lat_in, lon_in, t_in, dlat_in, dlon_in,
      lat_prev, lon_prev, t_prev, lat_next, lon_next, t_next,
      lat_int, lon_int, dlat_int, dlon_int, distance, dst_lat, dst_lon,
      delta_lat, delta_lon, delta_norm, delta_norm2, angle;
    
    // Doing the math
    lat_in = lat1[it->first];
    lon_in = lon1[it->first];
    if (lat_in < EPSILON && lon_in < EPSILON && !all) continue;
    t_in = t1[it->first];
    dlat_in = GEODESIC_DEG_TO_M*lat_in;
    dlon_in = GEODESIC_DEG_TO_M*cos(lat_in*DEG_TO_RAD)*lon_in;

    lat_prev = lat2[it->second.first];
    lon_prev = lon2[it->second.first];
    t_prev = t2[it->second.first];

    lat_next = lat2[it->second.second];
    lon_next = lon2[it->second.second];
    t_next = t2[it->second.second];

    if (t_next - t_prev < EPSILON) {         // next and prev may coincide
      lat_int = lat_prev;
      lon_int = lon_prev;
    }
    else {
      lat_int = mapping(t_in, t_prev, t_next, lat_prev, lat_next);
      lon_int = mapping(t_in, t_prev, t_next, lon_prev, lon_next);
    }

    dlat_int = GEODESIC_DEG_TO_M*lat_int;
    dlon_int = GEODESIC_DEG_TO_M*cos(lat_int*DEG_TO_RAD)*lon_int;

    dst_lat = dlat_in - dlat_int;
    dst_lon = dlon_in - dlon_int;
    distance = std::sqrt(dst_lat*dst_lat + dst_lon*dst_lon);

    delta_lat = lat_int - lat_in;
    delta_lon = lon_int - lon_in;
    delta_norm = std::sqrt(delta_lat*delta_lat + delta_lon*delta_lon);
    delta_norm2 = fabs(delta_lon);
    angle = RAD_TO_DEG*acos( delta_lon*fabs(delta_lon) / (delta_norm * delta_norm2) );
    
    if (delta_lat < 0) angle = 360.0 - angle;
//    angle = 90.0 - angle;

    // Filling JSON object
    input_gnss_coordinates["lat"] = lat_in;
    input_gnss_coordinates["lon"] = lon_in;
    input_gnss_coordinates["timestamp"] = t_in;

    bounding_coordinates["prev_lat"] = lat_prev;
    bounding_coordinates["prev_lon"] = lon_prev;
    bounding_coordinates["prev_timestamp"] = t_prev;
    bounding_coordinates["next_lat"] = lat_next;
    bounding_coordinates["next_lon"] = lon_next;
    bounding_coordinates["next_timestamp"] = t_next;
    bounding_coordinates["int_lat"] = lat_int;
    bounding_coordinates["int_lon"] = lon_int;

    jsoncons::json final_record;
    final_record["input_gnss_coordinate"] = std::move(input_gnss_coordinates);
    final_record["distance_from_gnss_coordinates"] = std::move(bounding_coordinates);
    final_record["distance"] = distance;
    final_record["dst_lat"] = dst_lat;
    final_record["dst_lon"] = dst_lon;
    final_record["timestamp"] = t_in;
    final_record["counter"] = counter;
    final_record["angle"] = angle;
 
    gps_records_distance.add(final_record);
  }

  //Generating JSON distance file
  output_file << jsoncons::pretty_print(gps_records_distance);
  output_file.close();

  return 0;
}

