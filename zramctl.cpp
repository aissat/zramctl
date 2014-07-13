/*
 * Developed by Timofey Titovets nefelim4ag@gmail.com
 * Licensed under GPL
 * zramctl simple tool to setup zram devices
 */

#include <iostream>
#include <string>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

void help();
bool used(string path);
void write(string path, string data);
string read(string path);
bool dir_exist(string path);
string find_free();
void status();

int main(int argc, char* argv[]) {
  int count=argc-1;
  string name="-h";
  string path="/sys/block/";
  if (count>0) name=argv[1];
  if (name.find("-h") != string::npos) {
    help();
    return 1;
  }
  if (!dir_exist(path+"zram0")) system("modprobe zram");
  if (name == "status") { status(); return 0; }
  else if (name == "reset" && count>=2) {
    for (int i=2; i<=count; i++) {
      if (!dir_exist(path+argv[i])) {
        cout << "can't find " << path+argv[i] << "\n"; return 1;
      }
      write(path+argv[i]+"/reset", "1");
    }
    return 1;
  }
  else if (name.find("zram") != string::npos) name=name;
  else if (name.find("find") != string::npos) name=find_free();
  path+=name;
  if (!dir_exist(path)) { cout << "can't find " << path << "\n"; return 1; }
  if (count>=2) write(path+"/reset", "1");
  if (count==4) write(path+"/max_comp_streams", argv[4]);
  if (count>=3) {
    string alg = argv[3];
    if (alg == "lzo" || alg == "lz4") write(path+"/comp_algorithm", alg);
    else  { cout << "Only lzo or lz4 allowed"; return 1; }
  }
  if (count>=2) write(path+"/disksize", argv[2]);
return 0;
}

void help(){
  cout
  << "Usage: zramctl <name> <size <alg> <threads> \n"
  << "zramctl zram0 1024M lz4 4                   \n"
  << "zramctl find  1024M lz4 4                   \n"
  << "zramctl reset zram0 zram1 ...               \n"
  << "zramctl status                              \n"
  << "lzo|lz4    # compress algorithm             \n"
  << "*|{K|M|G}  # size of zram disk              \n"
  << "<name>     # zram* or find                  \n"
  << "           # if find, print and setup first free device\n";
}

bool dir_exist(string path){
  struct stat info;
  if( stat( path.c_str(), &info ) != 0 ) return 0;
  if( info.st_mode&S_IFDIR ) return 1;
  return 0;
}

void write(string path, string data){
  ofstream file;
  file.open(path);
  file << data;
  file.close();
}

string read_file(string path) {
  ifstream file;
  file.open(path);
  string data, tmp;
  while(true) {
    file >> tmp;
    if(file.eof()) break;
    data=data+tmp+" ";
  }
  file.close();
  return data;
}

string find_free(){
  string name, path, _i;
  for (int i=0; i<32; i++) {
    _i=to_string(i);
    path="/sys/block/zram"+_i;
    if(!dir_exist(path)) break;
    if (!used(path+"/disksize")) {
      name="zram"+_i;
      cout << name << "\n";
      return name;
    }
  }
  return "all_used";
}

bool used(string path) {
  ifstream disksize;
  disksize.open(path);
  if (char(disksize.get()) == '0') return 0;
  else return 1;
}

void status() {
  printf("%5s %12s %10s %10s %10s %4s \n",
         "NAME",
         "DISKSIZE",
         "ORIG",
         "COMPRES",
         "ALGORITHM",
         "THR"
        );
  string name, disksize, path, algorithm, _i;
  for (int i=0;i<32;i++){
    _i=to_string(i);
    path="/sys/block/zram"+_i;
    name="zram"+_i;
    if (!dir_exist(path)) break;

    disksize=read_file(path+"/disksize");
    if (disksize[0]=='0') continue;

    algorithm=read_file(path+"/comp_algorithm");
    if (algorithm.find("[lzo]") != string::npos) algorithm = "lzo";
    else if(algorithm.find("[lz4]") != string::npos) algorithm = "lz4";

    printf("%5s %12s %10s %10s %10s %4s \n",
           name.c_str(),
           disksize.c_str(),
           read_file(path+"/orig_data_size").c_str(),
           read_file(path+"/compr_data_size").c_str(),
           algorithm.c_str(),
           read_file(path+"/max_comp_streams").c_str()
          );
  }
}