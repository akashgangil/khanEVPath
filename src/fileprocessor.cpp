#include "Python.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>
#include <map>
#include <queue>
#include <utility>
#include <unistd.h>
#include <vector>
#include "database.h"
#include "utils.h"
#include "log.h"
#include "measurements.h"
#include "fileprocessor.h"

extern struct stopwatch_t* sw;
extern FILE* mts_file;

static int execute_once = 0;

static std::string primary_attribute = "";

PyObject *pName, *pModule, *pDict, *pClass;

int init_python_processing(std::string script_name){
  if(!execute_once){

    log_info("**Executed**");

    //TODO: Remove it from there to some general init function

    pName = PyString_FromString(script_name.c_str());
    if(pName != NULL){
      pModule = PyImport_Import(pName);
    }
    if(pModule != NULL){
      pDict = PyModule_GetDict(pModule);
      if(pDict != NULL){
        pClass = PyDict_GetItemString(pDict, script_name.c_str());
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
    execute_once = 1;
    return 1;
  }
  return 0;

}

void cleanup_python(){
  Py_DECREF(pName);
  Py_DECREF(pDict);
  Py_DECREF(pModule);
  Py_DECREF(pClass);
}

std::string call_pyfunc(std::string func_name, PyObject *pInstance,
                        std::string format_str, std::string arg1, 
                        std::string arg2){

  std::string result;

  PyObject *pValue;

  if (PyCallable_Check(pClass))
  {
    char *func = strdup(func_name.c_str());
    if(format_str.empty()){
      pValue = PyObject_CallMethod(pInstance, func, NULL);
    }
    else{
      char *cformat_str = strdup(format_str.c_str());
      char *carg1 = strdup(arg1.c_str());
      char *carg2 = strdup(arg2.c_str());
      pValue = PyObject_CallMethod(pInstance, func, cformat_str, carg1, carg2);
    }
    
    if(pValue != NULL)
    {
      result = PyString_AsString(pValue);
      Py_DECREF(pValue);
    }
    else {
      if(PyErr_Occurred())
        PyErr_Print();
    }
    free(func);
  }

  return result;
}

void process_file(std::string server, std::string fileid, std::string file_path) {
  long double processing_time = 0;
  long double database_time = 0;
  log_info("Inside process file");
  std::string file = database_getval(fileid, "name");
  std::string ext = database_getval(fileid, "ext");
  file = server + "/" + file;

  std::string scripts = database_getvals("plugins");

  std::stringstream ss1 (scripts.c_str());
  std::string script_token = "";

  while(getline(ss1, script_token, ':')){
    if(strcmp(script_token.c_str(), "null")!=0){
      if(script_token == "Graph") continue;
      else{
        log_info("Processing %s", script_token.c_str());
        init_python_processing(script_token);
        std::string attrs=database_getval("plugins",script_token);

        if(attrs != "null"){
          std::string token="";
          std::stringstream ss2(attrs.c_str());

          PyObject *pFile, *pArgs, *pInstance;

          /*Initialize PyObjects Needed per file*/
          pFile = PyString_FromString(file_path.c_str());
          pArgs = PyTuple_New(1);
          PyTuple_SetItem(pArgs, 0, pFile);
          pInstance = PyObject_CallObject(pClass, pArgs);

          while(getline(ss2,token,':')){
            if(strcmp(token.c_str(),"null")!=0){
              if(token == "name" || token == "ext" || token == "location" ||
                  token == "experiment_id" || token == "file_path" || token == "tags") {
                continue;
              }

              stopwatch_start(sw);
              std::string res =  call_pyfunc(token, pInstance, "", "", "");
              stopwatch_stop(sw);
//              fprintf(mts_file, "ProcessFilePython:%s,%Lf,secs\n", token.c_str(), stopwatch_elapsed(sw));
              processing_time += stopwatch_elapsed(sw);
              log_info("Token: %s Result: %s", token.c_str(), res.c_str());

              stopwatch_start(sw);
              database_setval(fileid, token , res.c_str());
              stopwatch_stop(sw);
              database_time += stopwatch_elapsed(sw);
//            fprintf(mts_file, "ProcessFileDatabase:%s,%Lf,secs\n", token.c_str(), stopwatch_elapsed(sw));
            }
          }

          fprintf(mts_file, "ProcessFileDatabaseTime:%s,%Lf,secs\n", token.c_str(), database_time);
          fprintf(mts_file, "ProcessFileProcessingTime:%s,%Lf,secs\n", token.c_str(), processing_time);
          
          process_statistics(4, fileid, database_getval(fileid, "dbuffer1"), database_getval(fileid, "dmask1"));
          std::string destroy = "Destroy";
          call_pyfunc(destroy.c_str(), pInstance, "", "", "");
          log_info("Delete called");
          Py_DECREF(pArgs);
          Py_DECREF(pInstance);
        }
      }
    }
  }

}

void extract_attr_init(std::string file_path, int exp_id, std::string filepath) {

  char exp_id_str[10];
  sprintf(exp_id_str, "%d", exp_id);

  log_info("Extract attributed for %s", file_path.c_str());
  std::string ext = strrchr(filepath.c_str(),'.')+1;
  std::string filename=strrchr(filepath.c_str(),'/')+1;

  std::string fileid = database_setval("null","name",filename);
  database_setval(fileid,"ext",ext);
  database_setval(fileid,"server","test1");
  database_setval(fileid,"location","test2");
  database_setval(fileid,"file_path", filepath);
  database_setval(fileid,"experiment_id", exp_id_str);
  process_file("test1", fileid, file_path);
}

/*Rolling window conditional average*/
void RWCA(int window_size, std::string file_id, std::string dbuffer, std::string dmask){
  static std::queue < std::string > image_window;
  image_window.push(file_id);

  static std::map <std::string, std::pair<std::string, std::string> > image_data;
  image_data[file_id] = make_pair(dbuffer, dmask);
  printf("Queue size: %zu\n", image_window.size());
  
  if((int)image_window.size() < window_size) return;
  else if((int)image_window.size() == window_size){
    std::string mask_image_id = image_window.front();
    image_window.pop();
    std::string mask = image_data[mask_image_id].second;
    std::queue<std::string> current_window_images (image_window);
    std::string window_image_id = mask_image_id + " ";
  
    PyObject *pInstance, *pFile, *pArgs;
    /*Initialize PyObjects Needed per file*/
    pFile = PyString_FromString("");
    pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, pFile);
    pInstance = PyObject_CallObject(pClass, pArgs);
    
    std::string window_image_intensity = call_pyfunc("getIntensityFrame", pInstance, 
                                                      "(s s)", dbuffer.c_str(), dmask.c_str()) + " ";
    
    std::string base_image_id = current_window_images.front();
    current_window_images.pop();

    while(!current_window_images.empty()){
      std::string image_id = current_window_images.front();
      window_image_id += image_id + " " ;
      window_image_intensity += call_pyfunc("getIntensityFrame", pInstance, "(s s)", 
                                      image_data[image_id].first.c_str(), image_data[base_image_id].second.c_str()) + " ";
      current_window_images.pop();
    }

    Py_DECREF(pArgs);
    Py_DECREF(pInstance);
      
    database_setval("RWCA", window_image_id, window_image_intensity);
    image_data.erase(mask_image_id);
  }
}

typedef void (*fptr)(int, std::string, std::string, std::string);
std::vector<std::string> f_list;
std::map<std::string, fptr> f_map;

void process_statistics(int arg, std::string file_id, std::string dbuffer, std::string dmask){
  for(std::vector<std::string>::iterator it = f_list.begin(); it != f_list.end(); ++it)
    (*f_map[*it])(arg, file_id, dbuffer, dmask);
}  

void process_analytics_pipeline(std::string cwd) {
  f_map["RWCA"] = &RWCA;
  std::string script_name, line;
  std::string r_path = "/plugins/analytics.txt";
  std::string analytics_path = cwd + r_path;
  log_info("Analytics Path %s", analytics_path.c_str());
  std::ifstream transducers_file(analytics_path.c_str());
  getline(transducers_file, script_name);
  getline(transducers_file, line);
  const char *firstchar=line.c_str();
  while(firstchar[0]=='-') {
    std::stringstream ss(line.c_str());
    std::string attr;
    std::string temp;
    ss >> temp;
    ss >> attr;
    attr=trim(attr);
    f_list.push_back(attr);
    database_setval("functions", "analytics", attr);
    getline(transducers_file,line);
    firstchar=line.c_str();
  }
  transducers_file.close();
}

void process_transducers(std::string server) {

  stopwatch_start(sw);
  if(server == "cloud") {
    return;
  }
  std::string script_name, file_type, line;
  char cwd[1024];
  std::string r_path = "/plugins/attributes.txt";
  std::string cwd_path = strdup(getcwd(cwd, sizeof(cwd)));
  std::string attribute_path = cwd_path + r_path;
  log_info("Attribute path %s", attribute_path.c_str());
  std::ifstream transducers_file(attribute_path.c_str());
  while(transducers_file.good()){
    getline(transducers_file, script_name);
    getline(transducers_file, file_type);
    database_setval("allfiles","types",file_type);
    database_setval(file_type, "attrs", "name");
    database_setval(file_type, "attrs", "tags");
    database_setval(file_type, "attrs", "location");
    database_setval(file_type, "attrs", "ext");
    database_setval(file_type, "attrs", "experiment_id");
    database_setval(file_type, "attrs", "file_path");

    std::string ext=file_type;
    /*  PRIMARY ATTRIBUTE CODE
        getline(transducers_file, line);
        std::stringstream s_uniq(line.c_str());
        std::string uniq_attr = "";
        getline(s_uniq, uniq_attr, '*');
        if(uniq_attr != ""){
        primary_attribute = uniq_attr;
        }
        */
    getline(transducers_file,line);
    const char *firstchar=line.c_str();
    while(firstchar[0]=='-') {
      std::stringstream ss(line.c_str());
      std::string attr;
      std::string temp;
      ss >> temp;
      ss >> attr;
      attr=trim(attr);
      database_setval(ext,"attrs",attr);
      database_setval("plugins", script_name, attr);
      getline(transducers_file,line);
      firstchar=line.c_str();
    }
  }
  transducers_file.close();
  stopwatch_stop(sw);
  fprintf(mts_file, "ProcessTransducers,%Lg,secs\n", stopwatch_elapsed(sw));
  fsync(fileno(mts_file));
  process_analytics_pipeline(cwd_path); 
}

