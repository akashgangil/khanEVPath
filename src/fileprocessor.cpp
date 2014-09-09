#include "Python.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>

#include "database.h"
#include "utils.h"

#include "fileprocessor.h"

static std::string primary_attribute = "";

std::string call_pyfunc(std::string script_name, std::string func_name, std::string file_path){

    std::string result;

    PyObject *pName, *pModule, *pDict, *pValue, *pArgs, *pClass, *pInstance;

    PyObject *pFile;

    pName = PyString_FromString(script_name.c_str());
    pModule = PyImport_Import(pName);

    pDict = PyModule_GetDict(pModule);
    pClass = PyDict_GetItemString(pDict, script_name.c_str());

    if (PyCallable_Check(pClass))
    {
        pFile = PyString_FromString(file_path.c_str());
        pArgs = PyTuple_New(1);
        PyTuple_SetItem(pArgs, 0, pFile);
        pInstance = PyObject_CallObject(pClass, pArgs);
    }

    pValue = PyObject_CallMethod(pInstance, strdup(func_name.c_str()), NULL);

    if(pValue != NULL)
    {
        result = PyString_AsString(pValue);
        Py_DECREF(pValue);
    }
    else {
        PyErr_Print();
    }

    // Clean up
    /*Py_DECREF(pModule);
      Py_DECREF(pValue);
      Py_DECREF(pFile);
      Py_DECREF(pArgs);
      Py_DECREF(pClass);
      Py_DECREF(pInstance);
    */
    
    return result;
}

void process_file(std::string server, std::string fileid, std::string file_path) {
    printf("inside process_file\n");
    std::string file = database_getval(fileid, "name");
    std::string ext = database_getval(fileid, "ext");
    file = server + "/" + file;
    std::string attrs=database_getval(ext,"attrs");

    char msg4[100];
    if(attrs != "null"){
        std::string token="";
        std::stringstream ss2(attrs.c_str());

        while(getline(ss2,token,':')){
            if(strcmp(token.c_str(),"null")!=0){
                if(token == "name" || token == "ext" || token == "location" ||
                        token == "experiment_id" || token == "file_path" || token == "tags") {
                    continue;
                }
                std::string res =  call_pyfunc("Khan", token, file_path);
                database_setval(fileid, token , res.c_str());
            }
        }
    }
}

void extract_attr_init(std::string file_path) {
    std::string ext = strrchr(file_path.c_str(),'.')+1;
    std::string filename=strrchr(file_path.c_str(),'/')+1;

    if(database_getval("name", filename) == "null" || 1) { 
        std::string fileid = database_setval("null","name",filename);
        database_setval(fileid,"ext",ext);
        database_setval(fileid,"server","test1");
        database_setval(fileid,"location","test2");
        database_setval(fileid,"file_path", file_path);
        process_file("test1", fileid, file_path);
    } else {
        std::string fileid = database_getval("name",filename);
        database_setval(fileid,"server","test1");
        database_setval(fileid,"location","test1");
    }   
}

void process_transducers(std::string server) {

    if(server == "cloud") {
        return;
    }
    std::string line;
    std::ifstream transducers_file(("/net/hu21/agangil3/KhanScripts/transducers.txt"));
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
}

