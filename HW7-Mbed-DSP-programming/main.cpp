 
#include "mbed.h"
#include "arm_math.h"
#include <cmath>
#include <cstdint>
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_magneto.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_accelero.h"
#include "dsp.h"

// main() runs in its own thread in the OS

//=========macro_define ==============//
#define SAMPLE_SIZE 320
#define BLOCK_SIZE 32


//=========macro_define ==============//

//==============global parameters====================//
const uint8_t block_size =BLOCK_SIZE;
const uint16_t sample_size =SAMPLE_SIZE;
const uint16_t blockNumber = SAMPLE_SIZE/BLOCK_SIZE;
static float32_t firStateF32[2 * BLOCK_SIZE + 29 - 1];
const float32_t firCoeffs32[29] = {
  -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f,
  -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f,
  +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
  +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f
};

const float32_t firCoeffs32_mk2[29] = {
  -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f,
  -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f,
  +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
  +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f
};
//==============global parameters====================//

int main()
{
   float sensor_value = 0;
   int16_t pDataXYZ[3] = {0};
   float pGyroDataXYZ[3] = {0};

   uint32_t i;
   uint32_t mem =0;
   arm_fir_instance_f32 S;
   arm_status arm_status;
   float32_t *input_buff,*output_buff;
   input_buff = new float32_t[sample_size];  //320
   output_buff =new float32_t[sample_size]; //320
   float32_t *input_ptr = &input_buff[0];
   float32_t *output_ptr = &output_buff[0];
   
    BSP_TSENSOR_Init();
    BSP_HSENSOR_Init();
    BSP_PSENSOR_Init();

    BSP_MAGNETO_Init();
    BSP_GYRO_Init();
    BSP_ACCELERO_Init();
    
    arm_fir_instance_f32 FIR_insrance;

     
    float32_t data_x =0;
    float32_t data_y= 0;
    float32_t data_z =0;
    arm_fir_init_f32(&FIR_insrance, 29, (float32_t *)&firCoeffs32[0], &firStateF32[0], block_size);
    while (mem ==0) {
         printf(" =================data start collecting ==================== \n" );
         for(i=0; i<sample_size; i++)  // input collecting data
         {
               BSP_ACCELERO_AccGetXYZ(pDataXYZ);
               data_x= float32_t(pDataXYZ[0]);
               data_y= float32_t(pDataXYZ[1]);
               data_z= float32_t(pDataXYZ[2]);
               float32_t temp = sqrt(data_z*data_z +data_y*data_y + data_x*data_x);
               input_buff[i] =temp;
               //printf("the data number %d --> X_MAG : %f, Y_MAG: %f, Z_MAG: %f ,result : %f\n",i,data_x,data_y,data_z,temp);
               printf("%f\n" ,temp);
         }
             
          for(i=0; i<blockNumber; i++)
         {
              arm_fir_f32( &FIR_insrance,input_ptr+(i*block_size),output_ptr+(i*block_size),block_size);
             
              //printf("X_MAG : %f, Y_MAG: %f, Z_MAG: %f\n",result);
         }
          printf(" =================result start printing==================== \n" );
         for(i=0; i<sample_size; i++) {
             printf(" %f \n",output_buff[i]);
         }
         mem ++;
        //ThisThread::sleep_for(10000); 
    }
}
 
// ===================================================lin's code=======================================================================
/*
#include "mbed.h"

#include "stm32l475e_iot01_gyro.h"
#include "dsp.h"

#define SAMPLE_SIZE     256    // 樣本大小，2 的冪次方

static float sin_out[SAMPLE_SIZE] = {0};
static float fft_input[SAMPLE_SIZE] = {0};  // 傅立葉轉換的輸入陣列
static float fft_output[SAMPLE_SIZE] = {0};     // 傅立葉轉換的輸出陣列
static float mag_output[SAMPLE_SIZE/2] = {0};

// 創建傅立葉轉換的實例
arm_rfft_fast_instance_f32 fft_instance;
arm_status status = arm_rfft_fast_init_f32(&fft_instance, SAMPLE_SIZE);

// 創建感測器的buffer
float pGyroDataXYZ[3] = {0};

// main() runs in its own thread in the OS
int main()
{
    // BSP_GYRO_Init();
    // printf("BSP_GYRO_Init\n");


    // 创建一个t=2S钟之内的采样数据，采样点数FFT_LENGTH，采样周期T=t/FFT_LENGTH，采样频率F=1/T
    for(uint16_t i = 0; i < SAMPLE_SIZE; i++)
    {
        sin_out[i] = arm_sin_f32(i*2*3.1416f/128) + 0.3f*arm_sin_f32(i*2*3.1416f/16);
        // sin_out[i] = arm_sin_f32(5*i*2*3.1416/360);
        printf("%f \n", sin_out[i]);
    }
    printf("start fft @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");

    arm_rfft_fast_f32(&fft_instance, sin_out, fft_output, 0);

    for (int i = 0; i < SAMPLE_SIZE; i++){
        // printf("mag_output[%d] = %.1f \n", i, mag_output[i]);
        printf("%f\n", fft_output[i]);
    }
    printf("start mag #######################################################################\n\n");


    arm_cmplx_mag_f32(fft_output, mag_output, SAMPLE_SIZE);

    for (int i = 0; i < SAMPLE_SIZE/2; i++){
        // printf("mag_output[%d] = %.1f \n", i, mag_output[i]);
        printf("%f\n", mag_output[i]);
    }
    printf("\n\n");


    // while (1){

    //     for (int i = 0; i < SAMPLE_SIZE; i++){
    //         // BSP_ACCELERO_AccGetXYZ(pDataXYZ);
    //         BSP_GYRO_GetXYZ(pGyroDataXYZ);
    //         fft_input[i] = pGyroDataXYZ[0];
    //         // printf("fft_input[%d] = %f ", i, fft_input[i]);
    //     }
    //     // printf("\n\n");

    //     arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);
    //     arm_cmplx_mag_f32(fft_output, mag_output, SAMPLE_SIZE);

    //     // // 印出轉換後的結果           
    //     // for (int i = 0; i < SAMPLE_SIZE; i++){
    //     //     printf("fft_output[%d] = %f ", i, fft_output[i]);
    //     // }
    //     // printf("\n\n");

    //     // 印出mag後的結果
    //     for (int i = 0; i < SAMPLE_SIZE; i++){
    //         printf("mag_output[%d] = %.1f \n", i, mag_output[i]);
    //     }
    //     printf("\n\n");

    //     ThisThread::sleep_for(10000ms); // 每次迴圈之間暫停 1000 毫秒（1 秒）
    // }
}
*/
/* ----------------------------------------------------------------------
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 *
* $Date:         17. January 2013
* $Revision:     V1.4.0
*
* Project:       CMSIS DSP Library
 * Title:        arm_fir_example_f32.c
 *
 * Description:  Example code demonstrating how an FIR filter can be used
 *               as a low pass filter.
 *
 * Target Processor: Cortex-M4/Cortex-M3
 *
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*   - Neither the name of ARM LIMITED nor the names of its contributors
*     may be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
 * -------------------------------------------------------------------- */
/* ----------------------------------------------------------------------
** Include Files
** ------------------------------------------------------------------- */
/*
#include "arm_math.h"
#include "math_helper.h"
#include "mbed.h"
#if defined(SEMIHOSTING)
#include <stdio.h>
#endif
/* ----------------------------------------------------------------------
** Macro Defines
