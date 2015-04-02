
#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H
#include <vector>

void extract_attr_init(std::string, int, std::string);
void process_transducers(std::string);
void process_statistics(int, std::string, std::string, std::string);
void process_analytics_pipeline(std::string);
void process_file(std::string, std::string, std::string);
int process_python_code(std::string py_script, std::string py_function, std::string file_name, char * db_id);
void cleanup_python();
void initialize_attrs_for_data_types(const std::vector<std::string> & types);
#endif
