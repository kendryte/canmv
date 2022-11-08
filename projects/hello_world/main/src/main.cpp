/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <bsp.h>
#include <sysctl.h>
#include "FreeRTOS.h"
#include "task.h"

#define TASK_STACK_SIZE (32 * 1024)
#define TASK_STACK_LEN (TASK_STACK_SIZE / sizeof(StackType_t))
void * __dso_handle = 0 ;
TaskHandle_t task_1_handle;

int core1_function(void *ctx)
{
    uint64_t core = current_coreid();
    printf("Core %ld Hello world\n", core);
    //vTaskStartScheduler();
    //while(1);
    return 0;
}

void task_1(void* arg)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    while(1)
    {
        printf("---1---\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    uint64_t core = current_coreid();
    int data;
    printf("Core %ld Hello world\n", core);
    register_core1(core1_function, NULL);

    xTaskCreateAtProcessor(0,                     // processor
                            task_1,               // function entry
                            "task_1",             //task name
                            TASK_STACK_LEN,     //stack_deepth
                            NULL,               //function arg
                            5,      //task priority
                            &task_1_handle); //task handl
    vTaskStartScheduler();
    for (;;)
        ;

    return 0;
}

