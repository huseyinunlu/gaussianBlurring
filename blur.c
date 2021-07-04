#include "lodepng.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct threadVars{
  int starty;
  int finishy;
};

unsigned char* image = 0;
unsigned width, height;
unsigned char** splittedImage;
unsigned char** outputSplittedImage;
unsigned char* mergedImage;
int size;

pthread_mutex_t mutex;

void decode(const char* filename) {
  unsigned error;
  error = lodepng_decode32_file(&image, &width, &height, filename);
  if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
}


void splitImage(){
  //memory allocation and splitting image lines to splitted image list
  splittedImage = (unsigned char**)malloc(height*sizeof(char*));
  int i,j;
  int k=0;

  for(i=0;i<height;i++){
    splittedImage[i] = (unsigned char*)malloc(width*4*sizeof(char));
    for(j=0;j<width*4;j++){
      splittedImage[i][j] = image[k];
      k++;
    }
  }
}

void * routine(void * args){
  struct threadVars* indexes = (struct threadVars*)args;
  int rMean,gMean,bMean;
  for(int i=indexes->starty;  i < indexes->finishy;i++){
    for(int j=0;j<width*4;j+=4){
      int rSum =0;
      int gSum =0;
      int bSum=0;
      int a=0;
      int div=9;
      int xs = -1;
      int xf = 2;
      int ys = -1;
      int yf = 2;
      if(i==0){
        xs = 0;
        a++;
      }
      if(j==0){
        ys = 0;
        a++;
      }
      if(i==height-1){
        xf = 1;
        a++;
      }
      if(j==(width-1)*4){
        yf =1;
        a++;
      }
      for(int k=xs;k<xf;k++){
        for(int l=ys;l<yf;l++){
          rSum += (int)splittedImage[i+k][j+(l*4)];
          gSum += (int)splittedImage[i+k][j+(l*4)+1];
          bSum += (int)splittedImage[i+k][j+(l*4)+2];
        }
      }
      if(a ==1){
        div = 6;
      }
      if(a==2){
        div = 4;
      }
      rMean = rSum/div;
      gMean = gSum/div;
      bMean = bSum/div;
      for(int k=xs;k<xf;k++){
        for(int l=ys;l<yf;l++){
          pthread_mutex_lock(&mutex);
          outputSplittedImage[i+k][j+(l*4)] = (unsigned char) rMean;
          outputSplittedImage[i+k][j+(l*4)+1] = (unsigned char) gMean;
          outputSplittedImage[i+k][j+(l*4)+2] = (unsigned char) bMean;
          outputSplittedImage[i+k][j+(l*4)+3] = splittedImage[i+k][j+(l*4)+3];
          pthread_mutex_unlock(&mutex);
        }
      }
    }
  }
}


void mergeImage(){
  int i,j;
  int k=0;

  for(i=0;i<height;i++){
    
    for(j=0;j<width*4;j++){
      image[k] = outputSplittedImage[i][j];
      k++;
    }
  }
}



void encode(const char* filename) {
  /*Encode the image*/
  unsigned error = lodepng_encode32_file(filename, image, width, height);

  /*if there's an error, display it*/
  if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
  

}


int main(int argc, char *argv[]) {
  if(argc !=4){printf("Error, invalid arguments."); exit(0);}
  const char* filename = argv[1];
  const int threadCount = atoi(argv[2]);
  const int blurTimes = atoi(argv[3]);
  const char* outputname = "output.png";
  
  pthread_t threads[threadCount];
  
  struct threadVars * threadVars;
  threadVars = (struct threadVars*)malloc(threadCount*sizeof(struct threadVars));


  decode(filename);
  splitImage();
  for(int a=0;a<blurTimes;a++){
    int i;
    int threadSize = height/threadCount;
    for(i=0;i<threadCount;i++){
      if(i==threadCount-1){
        threadVars[i].starty = i*threadSize;
        threadVars[i].finishy = height;
      }
      else{
        threadVars[i].starty = i*threadSize;
        threadVars[i].finishy = (i+1)*threadSize;
      }
      
    }


    outputSplittedImage = (unsigned char**)malloc(height*sizeof(char*));
    for(i=0;i<height;i++){
      outputSplittedImage[i] = (unsigned char*)malloc(width*4*sizeof(char));
      
    }



    pthread_mutex_init(&mutex, NULL);
    for(i = 0; i<threadCount;i++){
          if(pthread_create(&threads[i], NULL, &routine, &threadVars[i]) != 0){
              printf("Thread create failed");
          }
    }
    for(i = 0; i<threadCount;i++){
        if(pthread_join(threads[i], NULL) != 0){
            printf("Thread join failed");
        }
    }
    pthread_mutex_destroy(&mutex);
    splittedImage = outputSplittedImage;
  }
  

  mergeImage();

  encode(outputname);
  free(image);
  return 0;
}