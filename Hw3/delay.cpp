#include <iostream>
#include <unistd.h>
#include <cstdlib>

using namespace std;
int main(int argc,char** argv){
  int time = 3;
  if(argc >= 2){
    time = atoi(argv[1]);
  }
  cout << "Sleep for " << time << " seconds" << endl;
  for(int i=0;i<time;i++){
    sleep(1);
    cout << "Sleep: " << i+1 << endl;
  }
  cout << "Done!" << endl;
  return 0;
}

