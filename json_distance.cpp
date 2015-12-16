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

#include <map>

#include "jsoncons/json.hpp"

#define GEODESIC_DEG_TO_M         111070.4                              // conversion [deg] -> [meter]
#define RAD_TO_DEG                57.2957795131                         // 180/pi
#define DEG_TO_RAD                1.745329251e-2                        // pi/180

#define MAJOR_VERSION           1
#define MINOR_VERSION           0

double mapping(double x, double old_min, double old_max, double new_min, double new_max) {
  return (x - old_min) / (old_max - old_min)*(new_max - new_min) + new_min;
};

void usage(char* progname) {
  // Usage
  std::cout << "Usage: " << progname << " -i [input1.json] -d [input2.json] -o [output.json]" << std::endl;
  std::cout << "We distinguish between -i and -d because the program is going to calculate distance between points\n"
    << "in [input1.json] from a virtual point in [input2.json] at the same identical timestamp,\n"
    << "calculated interpolating points inside it" << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::cout << "********* JSON DISTANCE Calculator *********" << std::endl;
  std::cout << argv[0] << " v" << MAJOR_VERSION << "." << MINOR_VERSION << std::endl;

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
    for ( auto it = gps_records_1.begin_members(); it != gps_records_1.end_members(); it++) {
      lat1.push_back(it->value().has_member("lat") ? it->value()["lat"].as<double>() : 90.0);
      lon1.push_back(it->value().has_member("lon") ? it->value()["lon"].as<double>() : 90.0);
        t1.push_back(it->value().has_member("timestamp") ? it->value()["timestamp"].as<double>() : 90.0);
    }
  }

  if (gps_records_2.is_array()) {
    for (size_t i = 0; i < gps_records_2.size(); i++) {
      lat1.push_back(gps_records_2[i].has_member("lat") ? gps_records_2[i]["lat"].as<double>() : 90.0);
      lon1.push_back(gps_records_2[i].has_member("lon") ? gps_records_2[i]["lon"].as<double>() : 90.0);
      t1.push_back(gps_records_2[i].has_member("timestamp") ? gps_records_2[i]["timestamp"].as<double>() : 90.0);
    }
  }
  else if (gps_records_2.is_object()) {
    for (auto it = gps_records_2.begin_members(); it != gps_records_2.end_members(); it++) {
      lat2.push_back(it->value().has_member("lat") ? it->value()["lat"].as<double>() : 90.0);
      lon2.push_back(it->value().has_member("lon") ? it->value()["lon"].as<double>() : 90.0);
      t2.push_back(it->value().has_member("timestamp") ? it->value()["timestamp"].as<double>() : 90.0);
    }
  }

  std::cout << "analyze   : " << lat1.size() << "  " << lon1.size() << "  " << t1.size() << std::endl;
  std::cout << "benchmark : " << lat2.size() << "  " << lon2.size() << "  " << t2.size() << std::endl;

  std::map<int, std::pair<int, int> > time_map;
  for (int i = 0; i < t1.size(); i++) {
    int prev = -1, next = -1;
    for (int j = 0; j < t2.size(); j++) {
      if (t1[i] < t2[j] && prev == -1) prev = j-1;
      if (t1[i] > t2[t2.size() -1 - j] && next == -1) next = t2.size() - j;
    }
    if (prev == -1 || next == -1) continue;
    time_map[i] = std::make_pair(prev, next);
  }
  std::cout << "map size    : " << time_map.size() << std::endl;
  int testindex = 0;
  std::cout << "map example : " << testindex << " " << time_map[testindex].first << "," << time_map[testindex].second << " - " 
    << std::fixed << std::setprecision(3) << t1[testindex] << "  " << t2[time_map[testindex].first] << " , " << t2[time_map[testindex].second] << std::endl;
  testindex = 4000;
  std::cout << "map example : " << testindex << " " << time_map[testindex].first << "," << time_map[testindex].second << " - " 
    << std::fixed << std::setprecision(3) << t1[testindex] << "  " << t2[time_map[testindex].first] << " , " << t2[time_map[testindex].second] << std::endl;

  jsoncons::json gps_records_distance;

    
  /*
  // Distances comparison
  jsoncons::json gps_records_distance(jsoncons::json::an_array);
  if (gps_records_1.is_array()) {
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

        double temp_timestamp, time_distance;
        if (gps_records_2.is_array()) {
          time_distance = 1e9;
          for (size_t j = 0; j < gps_records_2.size(); ++j) {
            temp_timestamp = gps_records_2[j].has_member("timestamp") ? gps_records_2[j]["timestamp"].as<double>() : -1.0;
            if (temp_timestamp <= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), first_timestamp = temp_timestamp, first = j;
          }
          time_distance = 1e9;
          for (size_t j = gps_records_2.size() - 1; j >= 0; --j) {
            temp_timestamp = gps_records_2[j].has_member("timestamp") ? gps_records_2[j]["timestamp"].as<double>() : -2.0;
            if (temp_timestamp >= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), last_timestamp = temp_timestamp, last = j;
          }
          first_lat = gps_records_2[first].has_member("lat") ? gps_records_2[first]["lat"].as<double>() : 90.0;
          first_lon = gps_records_2[first].has_member("lon") ? gps_records_2[first]["lon"].as<double>() : 90.0;
          first_timestamp = gps_records_2[first].has_member("timestamp") ? gps_records_2[first]["timestamp"].as<double>() : -3.0;
          last_lat = gps_records_2[last].has_member("lat") ? gps_records_2[last]["lat"].as<double>() : 90.0;
          last_lon = gps_records_2[last].has_member("lon") ? gps_records_2[last]["lon"].as<double>() : 90.0;
          last_timestamp = gps_records_2[last].has_member("timestamp") ? gps_records_2[last]["timestamp"].as<double>() : -4.0;
        }
        else if (gps_records_2.is_object()) {
          size_t j;
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            temp_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -5.0;
            if (temp_timestamp <= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), first_timestamp = temp_timestamp, first = j;
          }
          std::cout << "j: " << j << std::endl;
          for (auto rec2 = gps_records_2.end_members(); rec2 != gps_records_2.begin_members(); --rec2, --j) {
            temp_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -6.0;
            if (temp_timestamp >= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), last_timestamp = temp_timestamp, last = j;
          }
          std::cout << "j: " << j << std::endl;
          std::cout << "first: " << first << ", last: " << last << std::endl;
//          std::cin.get();
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == first) {
              first_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              first_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              first_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -7.0;
              break;
            }
          }
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == last) {
              last_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              last_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              last_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -8.0;
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
  else if (gps_records_1.is_object()) {
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

        double temp_timestamp, time_distance;
        if (gps_records_2.is_array()) {
          time_distance = 1e9;
          for (size_t j = 0; j < gps_records_2.size(); ++j) {
            temp_timestamp = gps_records_2[j].has_member("timestamp") ? gps_records_2[j]["timestamp"].as<double>() : -1.0;
            if (temp_timestamp <= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), first_timestamp = temp_timestamp, first = j;
          }
          time_distance = 1e9;
          for (size_t j = gps_records_2.size() - 1; j >= 0; --j) {
            temp_timestamp = gps_records_2[j].has_member("timestamp") ? gps_records_2[j]["timestamp"].as<double>() : -2.0;
            if (temp_timestamp >= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), last_timestamp = temp_timestamp, last = j;
          }
          first_lat = gps_records_2[first].has_member("lat") ? gps_records_2[first]["lat"].as<double>() : 90.0;
          first_lon = gps_records_2[first].has_member("lon") ? gps_records_2[first]["lon"].as<double>() : 90.0;
          first_timestamp = gps_records_2[first].has_member("timestamp") ? gps_records_2[first]["timestamp"].as<double>() : -3.0;
          last_lat = gps_records_2[last].has_member("lat") ? gps_records_2[last]["lat"].as<double>() : 90.0;
          last_lon = gps_records_2[last].has_member("lon") ? gps_records_2[last]["lon"].as<double>() : 90.0;
          last_timestamp = gps_records_2[last].has_member("timestamp") ? gps_records_2[last]["timestamp"].as<double>() : -4.0;
        }
        else if (gps_records_2.is_object()) {
          size_t j;
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            temp_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -5.0;
            if (temp_timestamp <= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), first_timestamp = temp_timestamp, first = j;
          }
          for (auto rec2 = gps_records_2.end_members(); rec2 != gps_records_2.begin_members(); --rec2, --j) {
            temp_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -6.0;
            if (temp_timestamp >= input_timestamp && fabs(temp_timestamp - input_timestamp) < time_distance) time_distance = fabs(temp_timestamp - input_timestamp), last_timestamp = temp_timestamp, last = j;
          }
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == first) {
              first_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              first_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              first_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -7.0;
              break;
            }
          }
          j = 0;
          for (auto rec2 = gps_records_2.begin_members(); rec2 != gps_records_2.end_members(); ++rec2, ++j) {
            if (j == last) {
              last_lat = rec2->value().has_member("lat") ? rec2->value()["lat"].as<double>() : 90.0;
              last_lon = rec2->value().has_member("lon") ? rec2->value()["lon"].as<double>() : 90.0;
              last_timestamp = rec2->value().has_member("timestamp") ? rec2->value()["timestamp"].as<double>() : -8.0;
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
  */

  //Generating JSON distance file
  output_file << jsoncons::pretty_print(gps_records_distance);
  output_file.close();

  return 0;
}

