#include <iostream>
#include <fstream>
#include <string>
using namespace std;

struct Binder
{
	string subject;
	int xAxisMotion[50];
	int xPos, yPos, zPos;
	int lightingVal;
};

void getFileContent(string &bindersMotion, string filePath);

int main() {
	string initializationInfo;
	string binderInitializationInfo = "/home/pi/iotBinder/binderInitializationInfo.txt";
	int bindersPresent;
	
	string bindersMotion;
	string biMotion = "/home/pi/iotBinder/biMotion.txt";

	//char binderID;
	getFileContent(initializationInfo, binderInitializationInfo);
	cout << initializationInfo << endl;
	
	bindersPresent = initializationInfo[0] - '0';
	struct Binder binder[bindersPresent];
	cout << bindersPresent << endl;
	int binderID;
	string tempSubject;
	for(unsigned int i=1; i<initializationInfo.length(); i++){
		if(initializationInfo.at(i) == 'B'){
			tempSubject.clear();
			binderID = initializationInfo.at(i+1) - '0';
			cout << binderID << endl;
			for(unsigned int j = i +2; j<initializationInfo.length();j++){
				if(initializationInfo.at(j) == 'B'){
					i = j-1;//skip to next B
					break;
				}
				tempSubject +=initializationInfo.at(j);
			}
			cout << tempSubject<< endl;
			binder[binderID-1].subject = tempSubject;
			//cout << binder[binderID].subject << endl;
		}
	}
	
	getFileContent(bindersMotion, biMotion);
	
	cout << bindersMotion << endl;
	
	return 0;
}

void getFileContent(string &contents, string filePath)
{
	string line;
	ifstream myfile (filePath);
	if (myfile.is_open()){
		while(getline(myfile,line)){
			//cout << line << '\n';
			contents = contents.append(line);
			
		}
		myfile.close();
	}
	else cout << "Unable to open file";
}
