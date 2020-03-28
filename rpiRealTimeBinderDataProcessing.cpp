//make sure initialization file has content
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

struct Binder
{
	string subject;
	int xAxisMotion[50];
	int indexer =0;
	int xPos, yPos, zPos;
	int lightingVal; 
	bool inBag;
};

void getFileContent(string &bindersMotion, string filePath);
void fillBinderSubjects(vector<Binder> &binder, string &initializationInfo);
void fillBinderMotion(vector<Binder> &binder, string &binderMotion);
bool readIfMotionIsDetected();

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
	vector <Binder> binder;
	binder.resize(bindersPresent);
	
	fillBinderSubjects(binder,initializationInfo);
	
	bool inMotion;
	while (true){
		inMotion = readIfMotionIsDetected();
		if(inMotion){
			getFileContent(bindersMotion, biMotion);
		
			cout << bindersMotion << endl;
			fillBinderMotion(binder,bindersMotion);
			for(unsigned int i=0; i<50;i++){
				cout << "index" << i << endl;
				cout << "binder 1 " << binder.at(0).xAxisMotion[i] << endl;
				cout << "binder 2 " << binder.at(1).xAxisMotion[i] << endl;
			}
		}
		
		//if()
		
		break;
		
		/*
		
		*/
	}
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
//populates binder's subjects from a file 
void fillBinderSubjects(vector<Binder> &binder, string &initializationInfo){
	int binderID;//used as a temporary id for binder objects
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
			binder.at(binderID-1).subject = tempSubject;
			//cout << binder[binderID].subject << endl;
		}
	}
}
//populates the binder xAxisMotion array from file
void fillBinderMotion(vector<Binder> &binder, string &bindersMotion){
	int binderID;
	string motionVal;
	for(unsigned int i=0;i<bindersMotion.length();i++){
		if(bindersMotion.at(i)=='B'){
			binderID = bindersMotion.at(i+1)-'0';
			
			//add 3 to also account for the '.'
			for(unsigned int j=i+2;j<bindersMotion.length();j++){
				if(bindersMotion.at(j) == 'B'){
					i = j-1;
					break;	
				}
				if(bindersMotion.at(j)==','){
					binder.at(binderID-1).xAxisMotion[binder.at(binderID-1).indexer]=stoi(motionVal);
					binder.at(binderID-1).indexer++;
					motionVal.clear();
				}
				if(isdigit(bindersMotion.at(j))){
					motionVal += bindersMotion.at(j);
				}
			}
		}
	}
}
//read file which mpu.c writes if motion is detected
bool readIfMotionIsDetected(){
	bool motionDetected;
	string filePath = "/home/pi/iotBinder/isMotionDetected.txt";
	string line;
	ifstream myfile (filePath);
	if (myfile.is_open()){
		while(getline(myfile,line)){
			//cout << line << '\n';
			if(line.compare("1")==0){
				motionDetected = true;
				break;
			}
			else{
				motionDetected = false;
			}
		}
		myfile.close();
	}
	else cout << "Unable to open file";
	//clear file
	if(motionDetected){
		myfile.open(filePath, ios::out | ios::trunc);
		myfile.close();
	}
	
	return motionDetected;
}

