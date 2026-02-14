#pragma once

#include <stdint.h>
#include "./Input.h"
#include "esp_heap_caps.h"

class RetroDebugger
{
private:
  uint32_t lastSampleTime = 0;
  uint8_t numCores = 1;
  unsigned long monitorInterval;

public:

  void setup(){
    
  }

  void debug(){
    
    if(monitorInterval < input.now()){
    
      monitorInterval = input.now() + 500;
    
      // Free heap
      size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

      // Minimum free heap ever since boot
      // size_t minFreeHeap = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);

      // Max heap = free + used
      size_t usedHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT) - freeHeap;
      size_t maxHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);

      printf("$heap=%d/%d\n", usedHeap, maxHeap);

    }

  }

};

extern RetroDebugger debugger;