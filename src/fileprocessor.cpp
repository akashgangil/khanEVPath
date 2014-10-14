#include "Python.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <map>
#include <unistd.h>

#include <boost/log/trivial.hpp>

#include "database.h"
#include "utils.h"

#include "measurements.h"
#include "fileprocessor.h"

extern struct stopwatch_t* sw;
extern FILE* mts_file;

static std::string primary_attribute = "";

std::string call_pyfunc(std::string script_name, std::string func_name, std::string file_path){
  std::string result;

  PyObject *pName, *pModule, *pDict, *pValue, *pArgs, *pClass, *pInstance;

  PyObject *pFile;

  pName = PyString_FromString(script_name.c_str());
  if(pName != NULL){
    pModule = PyImport_Import(pName);
  }
  if(pModule != NULL){
    pDict = PyModule_GetDict(pModule);
    if(pDict != NULL){
      pClass = PyDict_GetItemString(pDict, script_name.c_str());
      if (PyCallable_Check(pClass))
      {
        pFile = PyString_FromString(file_path.c_str());
        pArgs = PyTuple_New(1);
        PyTuple_SetItem(pArgs, 0, pFile);
        pInstance = PyObject_CallObject(pClass, pArgs);
        Py_DECREF(pArgs);
        pValue = PyObject_CallMethod(pInstance, strdup(func_name.c_str()), NULL);

        if(pValue != NULL)
        {
          result = PyString_AsString(pValue);
          Py_DECREF(pValue);
        }
        else {
          Py_DECREF(pModule);
          Py_DECREF(pDict);
          Py_DECREF(pClass);
          Py_DECREF(pInstance);
          Py_DECREF(pFile);
          Py_DECREF(pName);
          if(PyErr_Occurred())
            PyErr_Print();
        }
      }
      else{
        if(PyErr_Occurred())
          PyErr_Print();
      }

    }
    else{
      if(PyErr_Occurred())
        PyErr_Print();
    }

  }
  else{
      if(PyErr_Occurred())
        PyErr_Print();
  }
  //Py_Finalize();


  // Clean up
  // Py_XDECREF(pModule);
  //  Py_XDECREF(pFile);
  //Py_XDECREF(pArgs);
  //Py_XDECREF(pClass);
  //Py_XDECREF(pInstance);


  return result;
}

void process_file(std::string server, std::string fileid, std::string file_path) {
  BOOST_LOG_TRIVIAL(info) << "Inside process file" << "\n"; 
  std::string file = database_getval(fileid, "name");
  std::string ext = database_getval(fileid, "ext");
  file = server + "/" + file;
  std::string attrs=database_getval(ext,"attrs");

  if(attrs != "null"){
    std::string token="";
    std::stringstream ss2(attrs.c_str());

    while(getline(ss2,token,':')){
      if(strcmp(token.c_str(),"null")!=0){
        if(token == "name" || token == "ext" || token == "location" ||
            token == "experiment_id" || token == "file_path" || token == "tags") {
          continue;
        }

        stopwatch_start(sw);
        std::string res =  call_pyfunc("Khan", token, file_path);
        stopwatch_stop(sw);
        fprintf(mts_file, "ProcessFilePython,%Lf,secs\n",stopwatch_elapsed(sw));

        BOOST_LOG_TRIVIAL(debug) << "Token: " << token << "Result: " << res;

        stopwatch_start(sw);
        database_setval(fileid, token , res.c_str());
        stopwatch_stop(sw);
        fprintf(mts_file, "ProcessFileDatabase,%Lf,secs\n", stopwatch_elapsed(sw));
      }
    }
  }
}

void extract_attr_init(std::string file_path, int exp_id) {

  char exp_id_str[10];
  sprintf(exp_id_str, "%d", exp_id);

  BOOST_LOG_TRIVIAL(info) << "Extract attributes for " << file_path; 
  std::string ext = strrchr(file_path.c_str(),'.')+1;
  std::string filename=strrchr(file_path.c_str(),'/')+1;

  std::string fileid = database_setval("null","name",filename);
  database_setval(fileid,"ext",ext);
  database_setval(fileid,"server","test1");
  database_setval(fileid,"location","test2");
  database_setval(fileid,"file_path", file_path);
  database_setval(fileid,"experiment_id", exp_id_str);
  process_file("test1", fileid, file_path);
}

void process_transducers(std::string server) {

  stopwatch_start(sw);
  if(server == "cloud") {
    return;
  }
  std::string line;
  std::ifstream transducers_file(("transducers.txt"));
  getline(transducers_file, line);
  while(transducers_file.good()){
    database_setval("allfiles","types",line);
    database_setval(line,"attrs","name");
    database_setval(line,"attrs","tags");
    database_setval(line,"attrs","location");
    database_setval("namegen","command","basename");
    database_setval(line,"attrs","ext");
    database_setval(line, "attrs", "experiment_id");
    database_setval(line, "attrs", "file_path");

    std::string ext=line;

    getline(transducers_file, line);
    std::stringstream s_uniq(line.c_str());
    std::string uniq_attr = "";
    getline(s_uniq, uniq_attr, '*');
    if(uniq_attr != ""){
      primary_attribute = uniq_attr;
    }

    getline(transducers_file,line);
    const char *firstchar=line.c_str();
    while(firstchar[0]=='-') {
      std::stringstream ss(line.c_str());
      std::string attr;
      getline(ss,attr,'-');
      getline(ss,attr,':');
      std::string command;
      getline(ss,command,':');
      attr=trim(attr);
      database_setval(ext,"attrs",attr);
      database_setval(attr+"gen","command",command);
      getline(transducers_file,line);
      firstchar=line.c_str();
    }
  }

  stopwatch_stop(sw);
  fprintf(mts_file, "ProcessTransducers,%Lg,secs\n", stopwatch_elapsed(sw));
  fsync(fileno(mts_file));
}

