
#include "Python.h"
#include "data_analytics.h"

#include <string>
#include <vector>

#include <boost/log/trivial.hpp>

#include "database.h"
#include "utils.h"

extern std::vector < std::string > server_ids;
extern std::vector < std::string > servers;
extern std::string primary_attribute;

extern char msg[4096];

void analytics(void) {

  BOOST_LOG_TRIVIAL(info) << "Analytics Called" << "\n";
  std::string experiments =  database_getvals(primary_attribute); 
  
  BOOST_LOG_TRIVIAL(info) << "Experiment Id's " << experiments;

  std::vector<std::string> experiment_list = split(experiments, ":");
  FILE *stream;
  for(int i=0; i<experiment_list.size(); ++i) {
    if(experiment_list[i] != "null") 
    {

      BOOST_LOG_TRIVIAL(info) << "Experiment Number: " << experiment_list[i];
      
      std::string vals = database_getval("Time", experiment_list[i]);
      
      BOOST_LOG_TRIVIAL(info) << "File Ids: " << vals;
      
      std::vector<std::string> exp_vec = split(vals, ":");

      std::string intensityframe1 = "";
      std::string intensityframe2 = "";

      for(int k=0; k<exp_vec.size(); k++) {
        intensityframe1 += database_getval(exp_vec[k], "IntensityFrame1") + " ";
        intensityframe2 += database_getval(exp_vec[k], "IntensityFrame2") + " ";
      }
      std::string exp_dir = "/net/hu21/agangil3/experiments/";

      std::string intensity_vals = intensityframe1 + "i " + intensityframe2; 

      PyObject *pName, *pModule, *pDict, *pValue, *pArgs, *pClass, *pInstance, *pIntensity, *pExperimentId;

      pName = PyString_FromString(strdup("Graph"));
      pModule = PyImport_Import(pName);

      pDict = PyModule_GetDict(pModule);
      pClass = PyDict_GetItemString(pDict, strdup("Graph"));

      if (PyCallable_Check(pClass))
      {
        pExperimentId = PyString_FromString(experiment_list[i].c_str());
        pIntensity = PyString_FromString(strdup(intensity_vals.c_str()));
        pArgs = PyTuple_New(2);
        PyTuple_SetItem(pArgs, 0, pExperimentId);
        PyTuple_SetItem(pArgs, 1, pIntensity);
        pInstance = PyObject_CallObject(pClass, pArgs);
      }

      PyObject_CallMethod(pInstance, strdup("Plot"), NULL);
      PyObject_CallMethod(pInstance, strdup("Stats"),NULL);

      std::string filename = "experiment_" + experiment_list[i] + "_graph.png"; 
      if(database_getval("name", filename) == "null" || 1) {
        std::string fileid = database_setval("null","name",filename);
        database_setval(fileid,"ext","png");
        database_setval(fileid,"server",servers.at(0));
        database_setval(fileid,"file_path",exp_dir + filename);
        database_setval(fileid,"location",server_ids.at(0));
        database_setval(fileid, primary_attribute, experiment_list[i]);
      }

      filename = "experiment_" + experiment_list[i] + "_stats.txt"; 
      if(database_getval("name", filename) == "null" || 1) {
        std::string fileid = database_setval("null","name",filename);
        database_setval(fileid,"ext","txt");
        database_setval(fileid,"server",servers.at(0));
        database_setval(fileid,"file_path",exp_dir + filename);
        database_setval(fileid,"location",server_ids.at(0));
        database_setval(fileid, primary_attribute, experiment_list[i]);
      }

    }
  }           
}
