#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
 
int fd;
int acclX, acclY, acclZ;
 
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
  fd = wiringPiI2CSetup (0x68);
  wiringPiI2CWriteReg8 (fd,0x6B,0x00);//disable sleep mode
  printf("set 0x6B=%X\n",wiringPiI2CReadReg8 (fd,0x6B));

  FILE *fptr;
  FILE *fptr2;
  FILE *fptr3;

  int mpuX[200];
  int count = 0;
  int motionInstanceCounter = 0;
  int Xpos, Ypos, Zpos;

  bool inMotion = false;

  clock_t t;
  int t1;
  double timeTaken;

  while(1) {
     t = clock();
     t1=t;
     count ++;
     acclX = read_word_2c(0x3B);
     acclY = read_word_2c(0x3D);
     acclZ = read_word_2c(0x3F);

     acclX = (int)acclX / 1000;
     acclY = (int)acclY / 1000;
     acclZ = (int)acclZ / 1000;
     /*
     printf("X %d\n", acclX);
     printf("Y %d\n", acclY);
     printf("Z %d\n", acclZ);
     */
     if (count<200){
        mpuX[count] = acclX;
     }
     else {
        for(int i=1; i<200;i++){
           mpuX[i-1] = mpuX[i];
        }
        mpuX[199] = acclX;
        fptr = fopen("/home/pi/iotBinder/mpuXData.txt","w");
        for(int i=0; i<200;i++){
           fprintf(fptr,"%d\n",mpuX[i]);
        }
        fclose(fptr);
        motionInstanceCounter = 0;
        inMotion = false;
        for(int i=150;i<200;i++){
           if(abs(mpuX[i-1]-mpuX[i])<3){
         continue;
           }
           motionInstanceCounter ++;
           //writes to file if there is motion
           if (motionInstanceCounter >= 10){
         fptr2 = fopen("/home/pi/iotBinder/isMotionDetected.txt","w");
         fprintf(fptr2,"%d\n",1);
         fclose(fptr2);
         inMotion = true;
         break;
           }
        }
        if (!inMotion){
           Xpos = acclX;
           Zpos = acclZ;
           Ypos = acclY;
           fptr3 = fopen("/home/pi/iotBinder/piOrientation.txt","w");
           fprintf(fptr3,"X %d\nY %d\nZ %d\n", Xpos, Ypos, Zpos);
           fclose(fptr3);
        }
     }

     //Calulates how long it takes to run code above 
     //and sets a delay so the time elapsed will be 50ms
     t = clock() - t;
     timeTaken = ((double)t)/CLOCKS_PER_SEC;
     //printf("It took %f seconds \n", timeTaken);

     delay((int)(50-(int)(timeTaken*1000)));
     //timeTaken = (int)(50-(timeTaken*1000));
     //printf("total time taken %f\n",timeTaken);

  }

  return 0;
}
