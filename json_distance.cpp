/* Copyright 2015 - Stefano Sinigardi */

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
#include "jsoncons/json.hpp"

#define GEODESIC_DEG_TO_M 111070.4 // conversion [deg] -> [meter]
#define RAD_TO_DEG                57.2957795131                         // 180/pi
#define DEG_TO_RAD                1.745329251e-2                        // pi/180

#define MAJOR_VERSION           1
#define MINOR_VERSION           0

void usage(char* progname) {
  // Usage
  std::cout << progname << " v" << MAJOR_VERSION << "." << MINOR_VERSION << std::endl;
  std::cout << "Usage: " << progname << " -i [input1.json] -d [input2.json] -o [output.json]" << std::endl;
  std::cout << "We distinguish between -i and -d because the program is going to calculate distance between points\n"
    << "in [input1.json] from a virtual point in [input2.json] at the same identical timestamp,\n"
    << "calculated interpolating points inside it" << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::cout << "********* JSON DISTANCE Calculator *********" << std::endl;

  std::string input1_name, input2_name, output_name;
  if (argc > 2) { /* Parse arguments, if there are arguments supplied */
    for (int i = 1; i < argc; i++) {
      if ((argv[i][0] == '-') || (argv[i][0] == '/')) {       // switches or options...
        switch (tolower(argv[i][1])) {
        case 'i':
          input1_name = argv[++i];
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




  //Parsing JSON gps databases
  jsoncons::json gps_records_1 = jsoncons::json::parse_file(input1_name);
  jsoncons::json gps_records_2 = jsoncons::json::parse_file(input2_name);

  jsoncons::json gps_records_distance(jsoncons::json::an_array);


  if (gps_records_1.type() == 2) {        //array-style
    for (size_t i = 0; i < gps_records_1.size(); ++i) {
      try {
        double distance = 0.0;
        size_t first = 0, last = 0;
        double input_timestamp, first_timestamp = 0.0, last_timestamp = 0.0, interpolated_timestamp;
        double input_lat, input_lon, input_dlat, input_dlon,
          first_lat, first_lon, first_dlat, first_dlon,
          last_lat, last_lon, last_dlat, last_dlon,
          interpolated_lat, interpolated_lon, interpolated_dlat, interpolated_dlon;

        jsoncons::json input_gnss_coordinates;
        input_lat = gps_records_1[i].has_member("lat") ? gps_records_1[i]["lat"].as<double>() : 90.0;
        input_lon = gps_records_1[i].has_member("lon") ? gps_records_1[i]["lon"].as<double>() : 90.0;
        input_timestamp = gps_records_1[i].has_member("timestamp") ? gps_records_1[i]["timestamp"].as<double>() : 0.0;
        input_gnss_coordinates["lat"] = input_lat;
        input_gnss_coordinates["lon"] = input_lon;
        input_gnss_coordinates["timestamp"] = input_timestamp;

        input_dlon = GEODESIC_DEG_TO_M*cos(input_lat*DEG_TO_RAD)*input_lon;
        input_dlat = GEODESIC_DEG_TO_M*input_lat;

        double temp_timestamp;
        if (gps_records_2.type() == 2) {             //array-style
          for (size_t j = 0; j < gps_records_2.size(); ++j) {
            temp_timestamp = gps_records_2[j].has_member("timestamp") ? gps_records_2[j]["timestamp"].as<double>() : 0.0;
            if (((temp_timestamp - input_timestamp) <= 0.) && temp_timestamp >= first_timestamp) first_timestamp = temp_timestamp, first = j;
            if (((temp_timestamp - input_timestamp) >= 0.) && temp_timestamp <= last_timestamp) last_timestamp = temp_timestamp, last = j;
          }
          first_lat = gps_records_2[first].has_member("lat") ? gps_records_2[first]["lat"].as<double>() : 90.0;
          first_lon = gps_records_2[first].has_member("lon") ? gps_records_2[first]["lon"].as<double>() : 90.0;
          last_lat = gps_records_2[last].has_member("lat") ? gps_records_2[last]["lat"].as<double>() : 90.0;
          last_lon = gps_records_2[last].has_member("lon") ? gps_records_2[last]["lon"].as<double>() : 90.0;
        }
        else if (gps_records_2.type() == 1) {        //object-style
          size_t j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            temp_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : 0.0;
            if (((temp_timestamp - input_timestamp) <= 0.) && temp_timestamp >= first_timestamp) first_timestamp = temp_timestamp, first = j;
            if (((temp_timestamp - input_timestamp) >= 0.) && temp_timestamp <= last_timestamp) last_timestamp = temp_timestamp, last = j;
          }
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == first) {
              first_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              first_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              break;
            }
          }
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == last) {
              last_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              last_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              break;
            }
          }
        }

        first_dlon = GEODESIC_DEG_TO_M*cos(first_lat*DEG_TO_RAD)*first_lon;
        first_dlat = GEODESIC_DEG_TO_M*first_lat;
        last_dlon = GEODESIC_DEG_TO_M*cos(last_lat*DEG_TO_RAD)*last_lon;
        last_dlat = GEODESIC_DEG_TO_M*last_lat;

        interpolated_timestamp = input_timestamp;
        interpolated_dlat = ((interpolated_timestamp - first_timestamp) / (last_timestamp - first_timestamp)) * last_dlat - ((interpolated_timestamp - last_timestamp) / (last_timestamp - first_timestamp)) * first_dlat;
        interpolated_dlon = ((interpolated_timestamp - first_timestamp) / (last_timestamp - first_timestamp)) * last_dlon - ((interpolated_timestamp - last_timestamp) / (last_timestamp - first_timestamp)) * first_dlon;
        interpolated_lat = interpolated_dlat / GEODESIC_DEG_TO_M;
        interpolated_lon = interpolated_dlon / (GEODESIC_DEG_TO_M*cos(interpolated_lat*DEG_TO_RAD));

        distance = sqrt((interpolated_dlat - input_dlat)*(interpolated_dlat - input_dlat) + (interpolated_dlon - input_dlon)*(interpolated_dlon - input_dlon));

        jsoncons::json distance_from_gnss_coordinates;
        distance_from_gnss_coordinates["lat1"] = first_lat;
        distance_from_gnss_coordinates["lon1"] = first_lon;
        distance_from_gnss_coordinates["timestamp1"] = first_timestamp;
        distance_from_gnss_coordinates["lat2"] = last_lat;
        distance_from_gnss_coordinates["lon2"] = last_lon;
        distance_from_gnss_coordinates["timestamp2"] = last_timestamp;
        distance_from_gnss_coordinates["int_lat"] = interpolated_lat;
        distance_from_gnss_coordinates["int_lon"] = interpolated_lon;
        distance_from_gnss_coordinates["int_timestamp"] = interpolated_timestamp;

        jsoncons::json final_record;
        final_record["input_gnss_coordinate"] = std::move(input_gnss_coordinates);
        final_record["distance_from_gnss_coordinates"] = std::move(distance_from_gnss_coordinates);
        final_record["distance"] = distance;

        gps_records_distance.add(final_record);
      }
      catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
      }
    }
  }
  else if (gps_records_1.type() == 1) {                  //object-style
    int i = 0;
    for (auto rec = gps_records_1.begin_members(); rec != gps_records_1.end_members(); ++rec, ++i) {
      try {
        double distance = 0.0;
        size_t first = 0, last = 0;
        double input_timestamp, first_timestamp = 0.0, last_timestamp = 0.0, interpolated_timestamp;
        double input_lat, input_lon, input_dlat, input_dlon,
          first_lat, first_lon, first_dlat, first_dlon,
          last_lat, last_lon, last_dlat, last_dlon,
          interpolated_lat, interpolated_lon, interpolated_dlat, interpolated_dlon;

        jsoncons::json input_gnss_coordinates;
        input_lat = rec->value().has_member("lat") ? rec->value()["lat"].as<double>() : 90.0;
        input_lon = rec->value().has_member("lon") ? rec->value()["lon"].as<double>() : 90.0;
        input_timestamp = rec->value().has_member("timestamp") ? rec->value()["timestamp"].as<double>() : 0.0;
        input_gnss_coordinates["lat"] = input_lat;
        input_gnss_coordinates["lon"] = input_lon;
        input_gnss_coordinates["timestamp"] = input_timestamp;

        input_dlon = GEODESIC_DEG_TO_M*cos(input_lat*DEG_TO_RAD)*input_lon;
        input_dlat = GEODESIC_DEG_TO_M*input_lat;

        double temp_timestamp;
        if (gps_records_2.type() == 2) {             //array-style
          for (size_t j = 0; j < gps_records_2.size(); ++j) {
            temp_timestamp = gps_records_2[j].has_member("timestamp") ? gps_records_2[j]["timestamp"].as<double>() : 0.0;
            if (((temp_timestamp - input_timestamp) <= 0.) && temp_timestamp >= first_timestamp) first_timestamp = temp_timestamp, first = j;
            if (((temp_timestamp - input_timestamp) >= 0.) && temp_timestamp <= last_timestamp) last_timestamp = temp_timestamp, last = j;
          }
          first_lat = gps_records_2[first].has_member("lat") ? gps_records_2[first]["lat"].as<double>() : 90.0;
          first_lon = gps_records_2[first].has_member("lon") ? gps_records_2[first]["lon"].as<double>() : 90.0;
          last_lat = gps_records_2[last].has_member("lat") ? gps_records_2[last]["lat"].as<double>() : 90.0;
          last_lon = gps_records_2[last].has_member("lon") ? gps_records_2[last]["lon"].as<double>() : 90.0;
        }
        else if (gps_records_2.type() == 1) {        //object-style
          size_t j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            temp_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : 0.0;
            if (((temp_timestamp - input_timestamp) <= 0.) && temp_timestamp >= first_timestamp) first_timestamp = temp_timestamp, first = j;
            if (((temp_timestamp - input_timestamp) >= 0.) && temp_timestamp <= last_timestamp) last_timestamp = temp_timestamp, last = j;
          }
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == first) {
              first_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              first_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              break;
            }
          }
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == last) {
              last_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              last_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              break;
            }
          }
        }

        first_dlon = GEODESIC_DEG_TO_M*cos(first_lat*DEG_TO_RAD)*first_lon;
        first_dlat = GEODESIC_DEG_TO_M*first_lat;
        last_dlon = GEODESIC_DEG_TO_M*cos(last_lat*DEG_TO_RAD)*last_lon;
        last_dlat = GEODESIC_DEG_TO_M*last_lat;

        interpolated_timestamp = input_timestamp;
        interpolated_dlat = ((interpolated_timestamp - first_timestamp) / (last_timestamp - first_timestamp)) * last_dlat - ((interpolated_timestamp - last_timestamp) / (last_timestamp - first_timestamp)) * first_dlat;
        interpolated_dlon = ((interpolated_timestamp - first_timestamp) / (last_timestamp - first_timestamp)) * last_dlon - ((interpolated_timestamp - last_timestamp) / (last_timestamp - first_timestamp)) * first_dlon;
        interpolated_lat = interpolated_dlat / GEODESIC_DEG_TO_M;
        interpolated_lon = interpolated_dlon / (GEODESIC_DEG_TO_M*cos(interpolated_lat*DEG_TO_RAD));

        distance = sqrt((interpolated_dlat - input_dlat)*(interpolated_dlat - input_dlat) + (interpolated_dlon - input_dlon)*(interpolated_dlon - input_dlon));

        jsoncons::json distance_from_gnss_coordinates;
        distance_from_gnss_coordinates["lat1"] = first_lat;
        distance_from_gnss_coordinates["lon1"] = first_lon;
        distance_from_gnss_coordinates["timestamp1"] = first_timestamp;
        distance_from_gnss_coordinates["lat2"] = last_lat;
        distance_from_gnss_coordinates["lon2"] = last_lon;
        distance_from_gnss_coordinates["timestamp2"] = last_timestamp;
        distance_from_gnss_coordinates["int_lat"] = interpolated_lat;
        distance_from_gnss_coordinates["int_lon"] = interpolated_lon;
        distance_from_gnss_coordinates["int_timestamp"] = interpolated_timestamp;

        jsoncons::json final_record;
        final_record["input_gnss_coordinate"] = std::move(input_gnss_coordinates);
        final_record["distance_from_gnss_coordinates"] = std::move(distance_from_gnss_coordinates);
        final_record["distance"] = distance;

        gps_records_distance.add(final_record);
      }
      catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
      }
    }
  }


  //Generating JSON distance file
  output_file << jsoncons::pretty_print(gps_records_distance);
  output_file.close();

  return 0;
}

