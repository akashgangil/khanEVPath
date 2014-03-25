#include "Python.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <fstream>

#include "database.h"
#include "utils.h"

#include "fileprocessor.h"

static string primary_attribute = "";

string call_pyfunc(string script_name, string func_name, string file_path){

//    printf("inside python function FUNC NAME:  %s\n", func_name);
    printf("Python FUNCTION!\n");
    string result;

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

/*processes files: issues the mp3info command for the file
 *   and fills in the values of the attributes*/
void process_file(string server, string fileid, string file_path) {
    printf("inside process_file\n");
    string file = database_getval(fileid, "name");
    string ext = database_getval(fileid, "ext");
    file = server + "/" + file;
    string attrs=database_getval(ext,"attrs");

    char msg4[100];
    if(attrs != "null"){
        string token="";
        stringstream ss2(attrs.c_str());

        while(getline(ss2,token,':')){
            if(strcmp(token.c_str(),"null")!=0){
                if(token == "name" || token == "ext" || token == "location" ||
                        token == "experiment_id" || token == "file_path" || token == "tags") {
                    continue;
                }
                //printf("Token: %s %s\n", token.c_str(), file_path.c_str()); 
                //fflush(stdout);
                string res =  call_pyfunc("Khan",token, file_path);
                //printf("token:response  =  %s:%s\n", token.c_str(), res.c_str());
                database_setval(fileid, token , res.c_str());
            }
        }
    }
}



void extract_attr_init(std::string file_path) {
    printf("here1\n %s\n", file_path.c_str());

    std::string ext = strrchr(file_path.c_str(),'.')+1;
    std::string filename=strrchr(file_path.c_str(),'/')+1;

    printf("here2\n");

    if(database_getval("name", filename) == "null" || 1) { 
        std::string fileid = database_setval("null","name",filename);
        database_setval(fileid,"ext",ext);
        database_setval(fileid,"server","test1");
        database_setval(fileid,"location","test2");
        database_setval(fileid,"file_path", file_path);
        //for(int k=0; k<server_ids.size(); k++) {
        //   database_setval(fileid, server_ids.at(k), "0");
        //}    
        process_file("test1", fileid, file_path);
    
    } else {
        std::string fileid = database_getval("name",filename);
        database_setval(fileid,"server","test1");
        database_setval(fileid,"location","test1");
    }   
}

void process_transducers(string server) {

    //log_msg("Process Transducers\n");

    if(server == "cloud") {
        return;
    }
    string line;
    ifstream transducers_file(("/net/hu21/agangil3/KhanScripts/transducers.txt"));
    getline(transducers_file, line);
    while(transducers_file.good()){
      //  log_msg("=============== got type =   \n");
        database_setval("allfiles","types",line);
        database_setval(line,"attrs","name");
        database_setval(line,"attrs","tags");
        database_setval(line,"attrs","location");
        database_setval("namegen","command","basename");
        database_setval(line,"attrs","ext");
        database_setval(line, "attrs", "experiment_id");
        database_setval(line, "attrs", "file_path");

        string ext=line;

      //  log_msg("===Unique Attribute!=== ");
        getline(transducers_file, line);
        stringstream s_uniq(line.c_str());
        cout << "Line   " << line << endl;
        string uniq_attr = "";
        getline(s_uniq, uniq_attr, '*');
        if(uniq_attr != ""){
          primary_attribute = uniq_attr;
        }
        cout << primary_attribute << endl;

        getline(transducers_file,line);
        const char *firstchar=line.c_str();
        while(firstchar[0]=='-') {
            stringstream ss(line.c_str());
            string attr;
            getline(ss,attr,'-');
            getline(ss,attr,':');
            string command;
            getline(ss,command,':');
            //log_msg("============ checking attr = \n");
            //log_msg("============ checking command = \n");
            attr=trim(attr);
            database_setval(ext,"attrs",attr);
            database_setval(attr+"gen","command",command);
            getline(transducers_file,line);
            firstchar=line.c_str();
        }
    }
}

