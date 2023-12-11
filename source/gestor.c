/*Limitations:
 *    process name max 15 chars(with spaces)
 *    max 6 processes
 *    en un instante de tiempo solo puede entrar un proceso
 *    en un instante de tiempo pueden salir los procesos que sean
 *
 */

#include "c2d/base.h"
#include "c3d/types.h"
#include <citro2d.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <3ds.h>
#include <stdlib.h>

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240

struct memoryBlock{
  int id;
  char status[15];
  int reaminingTime;
  int size;
  int beginingTime;
  u32 color;
};
struct memoryBlock empty_memory = {-1,"empty",-1};
int NEXT_ID = 1;



bool allTheSame(struct memoryBlock memory[]){
  for(int i = 1; i < 20; i++){
    if(memory[i].id != memory[i-1].id)
      return 0; 
  }
  return 1;
}

void printText(u8 time,struct memoryBlock memory[]){
//Print actual time
  //printf("\x1b[%d;1H%d\x1b[K",time,time);
  printf("%d",time);

  u8 acumulated = 0;
  for(int i = 0; i < 20; i++){
    //Para no comparar el primero con ninguno
    if(i > 0){
      if(memory[i].id != memory[i-1].id){
        printf(" [%d %s %d]",(i-acumulated-1)*100,memory[i-1].status,(acumulated+1)*100);
        acumulated = 0;

        if(i == 19){
          printf(" [%d %s %d]",1900,memory[i].status,100);
        }

      }
      else{
        acumulated++;
      } 
      if(i==19 && allTheSame(memory))
        printf(" [%d %s %d]",(i-acumulated)*100,memory[i-1].status,(acumulated+1)*100);
      else if(i == 19 && memory[i].id == memory[i-1].id){
        printf(" [%d %s %d]",(i-acumulated+1)*100,memory[i-1].status,(acumulated+1)*100);
      }
    }
  }
  printf("\n");
}

void drawTriangles(struct memoryBlock memory[]){
  //Grey,Red,Gren,Blue,Yellow,Purple,Orange,Black
  u32 colors[8] = {C2D_Color32(0x60, 0x67, 0x6E, 0xFF),C2D_Color32(0xFF, 0x00, 0x00, 0xFF),C2D_Color32(0x00, 0xFF, 0x00, 0xFF),C2D_Color32(0x00, 0x00, 0xFF, 0xFF),C2D_Color32(0xcc,0xc9,0x2d,0xff),C2D_Color32(0x7c,0x2d,0xcc,0xff),C2D_Color32(0xe8,0x81,0x3c,0xff),C2D_Color32(0x00, 0x00, 0x00, 0xFF)};

  for(int i = 0; i < 20; i++){
    //Empty space, grey
    if(memory[i].id == -1){
      C2D_DrawRectSolid(i*(SCREEN_WIDTH/20),70,0,SCREEN_WIDTH/20,100,colors[0]);
    }
    else{
      C2D_DrawRectSolid(i*(SCREEN_WIDTH/20),70,0,SCREEN_WIDTH/20,100,memory[i].color);
    }

    //Draw lines between blocks of memory
    C2D_DrawLine(i*(SCREEN_WIDTH/20),70,colors[7],i*(SCREEN_WIDTH/20),170,colors[7],1.5,0);
  }
}
//Type == 0 --> primer hueco
//Type == 1 ---> siguiente hueco
//index is the variable that sets in which position begin to search an empyt_space
void findEmptySpace(bool type,struct memoryBlock memory[],struct memoryBlock process,u8 *index){
  u8 counter = 0; 
  u8 infinite_loop = 0; 

    for(int i = *index; i < 20; i++){
      if(memory[i].id == -1)
        counter++;
      else
       counter=0;

      if(counter == process.size/100){
        for(int j=i-counter+1;j<i+1;j++){
          memory[j] = process;
        }
        *index = i;
        return;
      }
      if(*index >= 19){
        *index = 0;
        infinite_loop++;
      }
      if(infinite_loop == 2){
        printf("\nNO EMPTY SPACE\n");
        *index = i;
        return;
      }
    } 
}


void reduceTimer(struct memoryBlock memory[]){
  for(int i = 0; i<20;i++){
    if(memory[i].reaminingTime == 0 && memory[i].id != -1){
      memory[i] = empty_memory;
    } 
    else if(memory[i].reaminingTime > 0 && memory[i].id != -1)
      memory[i].reaminingTime--;
  }
}


int main(int argc,char* argv[]){
	// Init libs
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);
  romfsInit();

  srand(time(NULL));
	// Create screens
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

  //Color to clear the screen (every refresh) like background
  u32 clrClear = C2D_Color32(0xFF, 0xD8, 0xB0, 0x68);

  
  //Create the empty memory
  struct memoryBlock memory[20]; 
  for(int i = 0; i < 20; i++){
    memory[i] = empty_memory;
  }

  //Inicialize the cursor in the top-left bottom screen
  printf("\x1b[1;1H\x1b[K");
  printf("\n  PRESS B TO CHANGE THE TYPE OF MEMORY\n\t\t\tDEFAULT PRIMER HUECO\n");
   
  //Get inputs from the file
  //FILE* f = fopen("romfs:/input.txt","r");
  FILE* f = fopen("sdmc:/3ds/input_memory.txt","r");
  char c;
  int lines=0;
  while((c = (char)fgetc(f))){
    if(c == '\n'){
      lines++; 
    }
    if(feof(f)){
      lines++;
      break;
    }

  }
  
  struct memoryBlock instructions[lines];
  fseek(f,0,SEEK_SET);
  char line[40];
  int loop=0; 

  while(fgets(line,sizeof(line),f) != NULL){
    sscanf(line, "%s %d %d %d",instructions[loop].status,&instructions[loop].beginingTime,&instructions[loop].size,&instructions[loop].reaminingTime);
    instructions[loop].color = C2D_Color32(rand()%256,rand()%256,rand()%256,255);
    instructions[loop].id = NEXT_ID;
    NEXT_ID++;
    loop++;
  }
  fclose(f);

  

  u8 time = 1;
  u8 loop_index = 0;

  //0 = primer hueco || 1 = siguiente hueco
  bool TYPE_OF_MEMORY = 0;
  u8 index = 0;





  //---------------Main Loop-------------------------
  while(aptMainLoop()){
    hidScanInput();

    u32 kDown = hidKeysDown();
    if(kDown & KEY_START)
      break;

    // Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, clrClear);
		C2D_SceneBegin(top);
    
    drawTriangles(memory);

    if(kDown & KEY_A){    
      if(instructions[loop_index].beginingTime == time){
        findEmptySpace(TYPE_OF_MEMORY,memory,instructions[loop_index],&index);
        //printf("\n%d\n",index);
        if(TYPE_OF_MEMORY == 0)
          index = 0;
        loop_index++;
      }
      reduceTimer(memory);
      printText(time,memory);
      time++;

    }

    //Switch the type of the memory
    if(kDown & KEY_B){
      if(TYPE_OF_MEMORY == 0){
        printf("\n\t\tSWITCHED TO SIGUIENTE HUECO\n");
        TYPE_OF_MEMORY = 1;
      }
      else{
        printf("\n\t\tSWITCHED TO PRIMER HUECO\n");
        TYPE_OF_MEMORY = 0;
      }
    }
      
    C3D_FrameEnd(0);

  }

  romfsExit();
  C2D_Fini();
  C3D_Fini();
  gfxExit();

  return 0;
}
