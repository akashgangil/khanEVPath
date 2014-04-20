#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector>
#include "database.h"
#include "localizations.h"

extern std::vector < std::string > servers;
extern std::vector < std::string > server_ids;

std::vector<std::string> get_all_files(std::string ext) {
  std::cout << " GET ALL FILES " << std::endl;
  std::vector<std::string> files;
  std::string mp3s = database_getvals("all_"+ext+"s");
  std::stringstream ss(mp3s);
  std::string filename;
  while(getline(ss, filename, ':')) {
    if(filename.compare("")!=0) {
      std::string fileid = database_getval("name",filename);
      files.push_back(fileid);
      std::cout << "added id:"<<fileid<<std::endl;
    }
  }
  std::cout << " DONE " << std::endl;
  return files;
}

std::string random_location(std::string fileid) {
  return servers.at(rand()%servers.size());	
}

std::string genre_location(std::string fileid) {
  //get genre of file
  std::string genre = database_getval(fileid, "genre");
  std::cout << "looking for "<<genre<<std::endl;
  std::vector<int> server_counts(servers.size());
  //for each file
  std::vector<std::string> files = get_all_files("mp3");
  for(int i=0; i<files.size(); i++) {
    //if genre = file genre
    if(fileid.compare(files.at(i))) {
      std::string file_genre = database_getval(files.at(i), "genre");
      std::cout << "file genre "<<file_genre<<std::endl;
      if(file_genre.compare(genre)==0) {
        std::cout << "match "<<std::endl;
        std::string server = database_getval(files.at(i), "server");
        for(int j=0; j<servers.size(); j++) {
          if(servers.at(j).compare(server)==0) {
            server_counts.at(j) = server_counts.at(j) + 1;
            std::cout << "new count "<<server_counts.at(j) << std::endl;
          }
        }
      }
    }
  }
  //place on highest count server
  int max_num = 0;
  std::string max ="";
  for(int j=0; j<servers.size(); j++) {
    std::cout << "looking at server " << j << " with count " << server_counts.at(j) << std::endl;
    if(server_counts.at(j)>max_num) {
      max_num = server_counts.at(j);
      max = servers.at(j);
    }
  }
  return max;
}

std::vector<std::string> get_all_attr_vals(std::string attr) {
  std::vector<std::string> vals;
  std::string mp3s = database_getvals(attr);
  std::stringstream ss(mp3s);
  std::string val;
  while(getline(ss, val, ':')) {
    if(val.compare("")!=0) {
      vals.push_back(val);
    }
  }
  return vals;
}


int get_attr_numeric_val(std::string attr, std::string val) {
  //get all attr vals
  std::vector<std::string> vals = get_all_attr_vals(attr);
  //sort
  //sort(vals.begin(), vals.end());
  //return index of val
  int index = 0;
  for(int i =0; i<vals.size(); i++) {
    if(vals.at(i).compare(val)==0) {
      index = i;
    }
  }
  return index;
}

std::string knn_location(std::string fileid) {
  //keep track of min distance file
  float min_dist = 1000000;
  std::string min_file = fileid;
  //for each file
  std::string ext = database_getval(fileid, "ext");
  std::vector<std::string> files = get_all_files(ext);
  for(int i=0; i<files.size(); i++) {
    if(files.at(i).compare(fileid)!=0) {
      std::vector<std::string> attrs = get_all_attr_vals(fileid);
      int sum = 0;
      //for each file attr
      for(int j =0; j<attrs.size(); j++) {
        // get files attr numeric vals
        std::string val = database_getval(files.at(i), attrs.at(j));
        int fv = get_attr_numeric_val(attrs.at(j), val);
        val = database_getval(fileid, attrs.at(j));
        int iv = get_attr_numeric_val(attrs.at(j), val);
        //subtract
        int diff = fv - iv;
        //sqr then add to sum
        sum += pow(diff, 2);
      }
      //sqrt sum = distance
      float dist = sqrt(sum);
      //keep track of min dist file
      if(dist < min_dist) {
        min_dist = dist;
        min_file = files.at(i);
      }
    }
  }
  
  std::cout << "KNN SELECTED A LOCATION "<<std::endl;
  std::cout << min_file << std::endl;
  //get closest file server
  std::string min_file_server = database_getval(min_file, "server");
  return min_file_server;
}

std::string get_location(std::string fileid) {
  std::string type = "random";
  if(type.compare("random")==0) {
    return random_location(fileid);
  } else if(type.compare("genre")==0) {
    return genre_location(fileid);
  } else if(type.compare("knn")==0) {
    return knn_location(fileid);
  } else {
    fprintf(stderr, "invalid localizing algorithm\n");
    exit(1);
  }
  return NULL;
}

void usage_localize() {
  //for all files
  std::vector<std::string> files = get_all_files("mp3");
  for(int i=0; i<files.size(); i++) {
    //for each server
    int max_usage=0;
    int max_server=0;
    std::cout << "looking at file " << i << std::endl;
    for(int j=0; j<server_ids.size(); j++) {
      std::cout << "looking at server " << j << std::endl; 
      //track max usage count server
      std::string res=database_getval(files.at(i), server_ids.at(j));
      int usage = atoi(res.c_str());
      std::cout << "this server usage " << usage << std::endl;
      if(usage>max_usage) {
        std::cout << "new max " << usage << std::endl;
        max_usage = usage;
        max_server = j;
      }
    }
    //move to server
    std::string filename = database_getval(files.at(i), "name");
    std::string current = database_getval(files.at(i), "server");
    std::string cur_path = current + "/" + filename;
    std::string dst_path = servers.at(max_server) + "/" + filename;
    if(cur_path.compare(dst_path)) {
      std::cout << "moving to server " << max_server << std::endl;
      std::string command="mv " + cur_path + " " + dst_path;
      FILE* stream=popen(command.c_str(),"r");
      pclose(stream);
      //rename(cur_path.c_str(), dst_path.c_str());
      database_setval(files.at(i), "server", dst_path);
    }
  }
  std::cout << "done usage localize " << std::endl;
}
