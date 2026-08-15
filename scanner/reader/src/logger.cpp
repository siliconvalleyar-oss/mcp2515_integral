#include "logger.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

Logger::Logger() {}
Logger::~Logger() { close(); }

bool Logger::open() {
    time_t now=time(nullptr); tm* t=localtime(&now);
    std::stringstream ss;
    ss<<"log_"<<(t->tm_year+1900)
      <<std::setw(2)<<std::setfill('0')<<(t->tm_mon+1)
      <<std::setw(2)<<std::setfill('0')<<t->tm_mday
      <<"_"<<std::setw(2)<<std::setfill('0')<<t->tm_hour
      <<std::setw(2)<<std::setfill('0')<<t->tm_min<<".csv";
    filename=ss.str();
    file=std::make_unique<std::ofstream>(filename);
    if(!file->is_open()) return false;
    *file<<"timestamp,key,value\n";
    return true;
}
void Logger::log(const std::string& k,const std::string& v){
    if(!file||!file->is_open())return;
    *file<<time(nullptr)<<","<<k<<","<<v<<"\n"; file->flush();
}
void Logger::log(const std::string& k,int v)    { log(k,std::to_string(v)); }
void Logger::log(const std::string& k,float v)  { log(k,std::to_string(v)); }
void Logger::log(const std::string& k,double v) { log(k,std::to_string(v)); }
void Logger::close(){
    if(file&&file->is_open()){file->flush();file->close();}
}
