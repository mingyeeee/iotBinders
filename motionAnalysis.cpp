#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <cstdlib>
 
#define MPU6050_GYRO_XOUT_H        0x43   // R
#define MPU6050_GYRO_YOUT_H        0x45   // R
#define MPU6050_GYRO_ZOUT_H        0x47   // R
 
#define MPU6050_PWR_MGMT_1         0x6B   // R/W
#define MPU6050_I2C_ADDRESS        0x68   // I2C
using namespace std;
static int fd;
int read_word_2c(int addr)
{
int val;
val = wiringPiI2CReadReg8(fd, addr);
val = val << 8;
val += wiringPiI2CReadReg8(fd, addr+1);
if (val >= 0x8000)
val = -(65536 - val);
 
return val;
}

int main()
{
    fd = wiringPiI2CSetup(MPU6050_I2C_ADDRESS);
    if (fd == -1)
        return 1;
 
    wiringPiI2CReadReg8 (fd, MPU6050_PWR_MGMT_1);
    wiringPiI2CWriteReg16(fd, MPU6050_PWR_MGMT_1, 0);
 
    int x,y,z;
 
    while(true)
    {
        x = read_word_2c(0x3B);
        y = read_word_2c(0x3D);
        z = read_word_2c(0x3F);
            
        x = ((int)x / 1000) + 42;
        y = ((int)y / 1000) + 42;
        z = ((int)z / 1000) + 42;

        cout << x << endl;
    }
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>

using namespace std;

struct Binder
{
    string subject;
    int xAxisMotion[50];
    int indexer =0;
    int xPos, yPos, zPos;
    int lightingVal;
    bool inBag=true;
};

struct Backpack{
    int motion[100];
    string rawmotion;
};

void getFileContent(string &bindersMotion, string filePath, bool clearFile);
void fillBinderSubjects(vector<Binder> &binder, string &initializationInfo);
void fillBinderMotion(vector<Binder> &binder, string &binderMotion);
bool readIfMotionIsDetected();
void analysisBinderMotion(vector<Binder> &binder, Backpack &backpack);
vector<int> checkForBinderUpdates(vector<Binder> binder);
void requestBinderMotion();

int main() {
    Backpack backpack;
    string initializationInfo;
    string binderInitializationInfo = "/home/pi/iotBinder/binderInitializationInfo.txt";
    int bindersPresent;

    string bindersMotion;
    string biMotion = "/home/pi/iotBinder/biMotion.txt";

    //char binderID;
    getFileContent(initializationInfo, binderInitializationInfo, false);
    cout << initializationInfo << endl;

    bindersPresent = initializationInfo[0] - '0';
    vector <Binder> binder;
    binder.resize(bindersPresent);
    cout << "number of binders present: " << binder.size() << endl;

    fillBinderSubjects(binder,initializationInfo);

    bool inMotion;
    while (true){
        inMotion = readIfMotionIsDetected();
        if(inMotion){
            requestBinderMotion();
            sleep(5);
            getFileContent(bindersMotion, biMotion, true);

            cout << bindersMotion << endl;
            fillBinderMotion(binder,bindersMotion);
            cout << "starting analysis" << endl;
            analysisBinderMotion(binder, backpack);
            cout << "done analysis" << endl;

        }

    }
    return 0;
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
void fillBinderMotion(vector<Binder> &binder, string &bindersMotion){
    int binderID;
    string motionVal;
    for(unsigned int i=0;i<159;i++){
        if(bindersMotion.at(i)=='B'){
            binderID = bindersMotion.at(i+1)-'0';

            //add 3 to also account for the '.'
            for(unsigned int j=i+2;j<159;j++){
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
    bool motionDetected = false;
    string filePath = "/home/pi/iotBinder/isMotionDetected.txt";
    string content;
    getFileContent(content, filePath, true);
    if(content.compare("1") == 0){
        motionDetected = true;
    }
    return motionDetected;
}

//updates binder struct boolean inBag
void analysisBinderMotion(vector<Binder> &binder, Backpack &backpack){
    cout << "analysis function" << endl;
    cout << "file" << endl;
    int tempindexer=0;
    cout << "geting mpudata" << endl;
    
    ofstream cppRequestRead;
    cppRequestRead.open ("/home/pi/iotBinder/cppRequestRead.txt",ios::out | ios::trunc);
    cppRequestRead << "1";
    cppRequestRead.close();
    
    //--------------------------------------
    string line;
    ifstream myfile ("/home/pi/iotBinder/mpuXData.txt");
    if (myfile.is_open()){
        while(getline(myfile,line)){
            //cout << line << '\n';
            backpack.rawmotion +=line;

        }
        myfile.close();
    }
    else cout<<"unable to open" << endl;
    cout << "mpuData " << backpack.rawmotion << endl;
    //populatint array with file content
    for(unsigned int j=0; j<backpack.rawmotion.length(); j+=2){

        backpack.motion[j/2]= (backpack.rawmotion.at(j)-'0')*10+(backpack.rawmotion.at(j+1)-'0');
        cout<< "piMotion " << backpack.motion[j/2]<<endl;
        tempindexer++;

    }
    cout<< "indexer: "<< tempindexer << endl;
    int withinThreshold;
    int threshold = 5;
    int matchingThreshold = 10;
    bool tempinbag=false;
    for(unsigned int b=0; b < binder.size();b++){
        for(auto i=0; i<50; i++){
            withinThreshold = 0;
            for(auto j=i; j<i+50; j++){
                if(abs(binder.at(b).xAxisMotion[j]-backpack.motion[j])<threshold){
                    withinThreshold++;
                }
            }
            cout << "similarity test "<< i << ": "<< withinThreshold << endl;
            if(withinThreshold >= matchingThreshold){
                binder.at(b).inBag =true;
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

//unused for now
//returns a vector with which binders have been updated
vector<int> checkForBinderUpdates(vector<Binder> &binder){
    vector<int> binderIDs;
    string content;
    string filePath ="/home/pi/iotBinder/binderUpdate.txt";
    getFileContent(content, filePath, true);
    int numOfBinders = content.at(0)-'0';
    binderIDs.resize(numOfBinders);
    int binderID, counter;
    string tempVal;

    for(unsigned int i=1; i< content.length();i++){
        if(content.at(i)=='B'){
            binderID = content.at(i+1)-'0';
            //file is formated - Xpos,Ypos,Zpos,lighting
            for(unsigned int j=i+2;j<content.length();j++){
                if(content.at(j)=='B'){
                    counter=0;
                    i =j-1;
                    break;
                }
                if(content.at(j)==','){
                    switch (counter){
                        case 0:
                            binder.at(binderID-1).xPos=stoi(tempVal);
                            break;
                        case 1:
                            binder.at(binderID-1).yPos=stoi(tempVal);
                            break;
                        case 2:
                            binder.at(binderID-1).zPos=stoi(tempVal);
                            break;
                        case 3:
                            binder.at(binderID-1).lightingVal=stoi(tempVal);
                            break;
                        default:
                            break;
                    }
                    counter++;
                }
                if(isdigit(content.at(j))){
                    tempVal += content.at(j);
                }
            }
        }
    }
    return binderIDs;
}


