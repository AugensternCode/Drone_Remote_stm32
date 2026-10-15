# FreeRTOS kernel for the Drone target

This directory contains the FreeRTOS kernel used by the `Drone` Keil project.

Expected layout:

```text
FreeRTOS/
  Source/
    include/
    portable/
      RVDS/ARM_CM3/
      MemMang/
```

The first port targets the STM32F103C8 Cortex-M3 and Keil ARMCC5. Keep the
kernel version fixed while learning and porting so build changes remain easy
to review and revert.
