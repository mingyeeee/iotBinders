#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <thread>

#define MPU6050_GYRO_XOUT_H        0x43   // R
#define MPU6050_GYRO_YOUT_H        0x45   // R
#define MPU6050_GYRO_ZOUT_H        0x47   // R
 
#define MPU6050_PWR_MGMT_1         0x6B   // R/W
#define MPU6050_I2C_ADDRESS        0x68   // I2C
using namespace std;

//shared variables between threads
static int stop=0;
static vector<int> piMPU;
static bool motionDetected = false;
//static bool notcopying = true;
//------------------mpu monitoring------------------------
static int fd;
int read_word_2c(int addr){
	int val;
	val = wiringPiI2CReadReg8(fd, addr);
	val = val << 8;
	val += wiringPiI2CReadReg8(fd, addr+1);
	if (val >= 0x8000)
	val = -(65536 - val);
	 
	return val;
}

int motionMonitoring(int (*read_word_2cPTR)(int)){
    cout << "starting motion monitoring thread" << "\n";
    fd = wiringPiI2CSetup(MPU6050_I2C_ADDRESS);
    if (fd == -1)
        return 1;
 
    wiringPiI2CReadReg8 (fd, MPU6050_PWR_MGMT_1);
    wiringPiI2CWriteReg16(fd, MPU6050_PWR_MGMT_1, 0);
 
    int x,y,z;
    
    vector<int>::iterator it;
    int motionDetectedCounter;
    int vsize;
    while(true){
	//while(notcopying){
        x = read_word_2c(0x3B);
        y = read_word_2c(0x3D);
        z = read_word_2c(0x3F);
            
        x = ((int)x / 1000) + 42;
        y = ((int)y / 1000) + 42;
        z = ((int)z / 1000) + 42;

        //cout << x << "\n";
	piMPU.push_back(x);
	if(piMPU.size() > 100){
	    it = piMPU.begin();
	    piMPU.erase(it);
	}
	
	if(piMPU.size() == 100){
	    motionDetectedCounter = 0;
	    vsize = piMPU.size()-1;
	    for(auto i=0; i<10; i++){
		if(abs(piMPU.at(vsize - i)-piMPU.at(vsize - (i+1))) >1){
		    motionDetectedCounter++;
		    if (motionDetectedCounter > 5){
			motionDetected = true;
			//cout << "motion detected" << "\n";
			
			break;
		    }
		}
	    }
	}
	if(stop ==1){
	    break;
	}
	sleep(0.05);
    //}
    //cout << "motion monitoring thread -----PAUSED------" << endl;
    }
}
//--------------------------stop all thread--------------------------------
void stopProgram(){
    //type 1 to stop
    cout << "stop program thread starting" << endl;
    cin >> stop;
}
//--------------------------binder motion----------------------------------
struct Binder
{
    string subject;
    int xAxisMotion[50];
    int indexer =0;
    int xPos, yPos, zPos;
    int lightingVal;
    bool inBag=true;
};

void getFileContent(string &bindersMotion, string filePath, bool clearFile);
void fillBinderSubjects(vector<Binder> &binder, string &initializationInfo);
void fillBinderMotion(vector<Binder> &binder);
void analysisBinderMotion(vector<Binder> &binder, vector<int> &piMPUsnapshot);
void requestBinderMotion();

int main(){
    thread mpuMonitoring(motionMonitoring, read_word_2c);
    thread stopAll(stopProgram);
    cout << "main thread" << "\n";
    //initialization for binder stuff
    string initializationInfo;
    string binderInitializationInfo = "/home/pi/iotBinder/binderInitializationInfo.txt";
    int bindersPresent;

    //char binderID;
    getFileContent(initializationInfo, binderInitializationInfo, false);
    cout << initializationInfo << endl;

    bindersPresent = initializationInfo[0] - '0';
    vector <Binder> binder;
    binder.resize(bindersPresent);
    cout << "number of binders present: " << binder.size() << endl;

    fillBinderSubjects(binder,initializationInfo);

    vector<int> piMPUsnapshot(piMPU);
    while(stop == 0){
	if(motionDetected){
	    cout << "motion detected" << "\n";
	    motionDetected = false;
	    requestBinderMotion();
	    cout << "binder motion requested" << "\n";
	    sleep(3.5);
	    //deep copy MPU data
	    piMPUsnapshot.clear();
	    cout << "copying piMPU" << "\n";
	    copy(piMPU.begin(), piMPU.end(), back_inserter(piMPUsnapshot));
	    cout << "retrieving binder motion data|| binder size" << binder.size() << "\n";
	    fillBinderMotion(binder);
	    cout << "starting analysis" << "\n";
	    analysisBinderMotion(binder, piMPUsnapshot);
	}
    }
    
    stopAll.join();
    mpuMonitoring.join();
}

void getFileContent(string &contents, string filePath, bool clearFile){
    string line;
    ifstream myfile (filePath);
    if (myfile.is_open()){
        while(getline(myfile,line)){
            //cout << line << '\n';
            contents +=line;

        }
        myfile.close();
    }
    else cout << "Unable to open file"<<endl;

    if(clearFile){
        //cout << "deleting contents" << endl;
        myfile.open(filePath, ios::out | ios::trunc);
        myfile.close();
    }
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
void fillBinderMotion(vector<Binder> &binder){
    cout << "retrieving binder data" << "\n";
    string bindersMotion;
    string biMotion = "/home/pi/iotBinder/biMotion.txt";
    getFileContent(bindersMotion, biMotion, true);
    cout << "starting assignment" << "\n";
    int binderID;
    string motionVal;
    //reset counters
    for(unsigned int i=0;i<binder.size(); i++){
	binder.at(i).indexer = 0;
    }
    //assign the data from the txt file to binder struct
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

//updates binder struct boolean inBag
void analysisBinderMotion(vector<Binder> &binder, vector<int> &piMPUsnapshot){

    cout << "analysis function" << "\n";

    int withinThreshold;
    int threshold = 5;
    int matchingThreshold = 10;
    bool tempinbag=false;
    for(unsigned int b=0; b < binder.size();b++){
        for(auto i=0; i<50; i++){
            withinThreshold = 0;
            for(auto j=i; j<i+50; j++){
                if(abs(binder.at(b).xAxisMotion[j]-piMPUsnapshot.at(j))<threshold){
                    withinThreshold++;
                }
            }
            cout << "similarity test "<< i << ": "<< withinThreshold << "\n";
            if(withinThreshold >= matchingThreshold){
                binder.at(b).inBag =true;
                
		for(auto j=i; j<i+50; j++){
		    cout << "binder: " << binder.at(b).xAxisMotion[j] << "pi MPU: " << piMPUsnapshot.at(j) << "\n";
		}
		cout << "binder in bag" << endl;
                tempinbag = true;
                break;
            }
        }
    }
    if(!tempinbag){
        cout << "not in bag"<< endl;
    }

}

void requestBinderMotion(){
    ofstream myfile;
    myfile.open ("/home/pi/iotBinder/piMotion.txt",ios::out | ios::trunc);
    myfile << "yes";
    myfile.close();
}
