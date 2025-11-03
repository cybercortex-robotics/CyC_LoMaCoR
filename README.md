# Real-time Localization and Mapping services for Collaborative aerial and ground Robots (LoMaCoR)

## Introduction

In this example, we use the following CyberCortex.AI filters:

| Component | License | Source | Notes |
| :--- | :---: | ----------: | :--- |
| **Filter_LoMaCoR_Maps** | 🌐 open-source | local | Implementation of the NavGraph communication protocol. |
| **Filter_LoMaCoR_Server** | 🌐 open-source | local | Server for managing map queries. |
| **Filter_LoMaCoR_Viz** | 🌐 open-source | local | LoMaCoR visualization. |
| **Filter_HW_RgbdCamera** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_HW_RgbdCamera/linux-gcc-x64-ubuntu-24/Filter_HW_RgbdCamera.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> win-msvc-x64 | Acquires images from an RGBD camera (Intel RealSense). |
| **Filter_HW_Imu** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_HW_Imu/linux-gcc-x64-ubuntu-24/Filter_HW_Imu.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> win-msvc-x64 | Acquires inertial data from an IMU (Intel RealSense). |
| **Filter_Comm_DataChannel** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Comm_DataChannel/linux-gcc-x64-ubuntu-24/Filter_Comm_DataChannel.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> <a href="https://www.cybercortex.ai/data/filters/Filter_Comm_DataChannel/win-msvc-x64/Filter_Comm_DataChannel.zip" target="_blank">win-msvc-x64</a> | Communication between distributed DataBlocks (including CyberCortex.AI Droids). |
| **Filter_Vision_VisualSlam** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Vision_VisualSlam/linux-gcc-x64-ubuntu-24/Filter_Vision_VisualSlam.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> win-msvc-x64 | Communication between distributed DataBlocks (including CyberCortex.AI Droids). |
| **Filter_Visualization_Sensing** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Visualization_Sensing/linux-gcc-x64-ubuntu-24/Filter_Visualization_Sensing.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> <a href="https://www.cybercortex.ai/data/filters/Filter_Visualization_Sensing/win-msvc-x64/Filter_Visualization_Sensing.zip" target="_blank">win-msvc-x64</a> | Visualization of input and output filters results. |
