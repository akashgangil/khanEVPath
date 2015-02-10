
#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

std::string call_pyfunc(std::string, std::string, std::string);
void extract_attr_init(std::string, int, std::string);
void process_transducers(std::string);
void process_statistics(int, std::string, std::string, std::string);
void process_analytics_pipeline(std::string);
void process_file(std::string, std::string, std::string);
void cleanup_python();
#endif
