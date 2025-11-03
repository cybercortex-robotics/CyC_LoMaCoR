# Real-time Localization and Mapping services for Collaborative aerial and ground Robots (LoMaCoR)

## Introduction

## CyberCortex.AI setup

<a href="https://www.cybercortex.ai/" target="_blank">CyberCortex.AI</a> is a lightweight real-time AI operating system designed for autonomous robots and complex automation. It operates directly on embedded hardware, enabling robots to process sensory data, perform decision-making, and execute actions efficiently.

This repository illustrates how to use CyberCortex.AI in a straightforward robot vision application. The example uses the <a href="https://github.com/cybercortex-robotics/inference" target="_blank">CyberCortex.AI.inference</a> system, as a submodule in the current repository.

In this example, we use four filters:

| Component | License | Source | Notes |
| :--- | :---: | ----------: | :--- |
| **Filter_LoMaCoR_Maps** | 🌐 open-source | local | Implementation of the NavGraph communication protocol. |
| **Filter_LoMaCoR_Server** | 🌐 open-source | local | Server for managing map queries. |
| **Filter_LoMaCoR_Viz** | 🌐 open-source | local | LoMaCoR visualization. |
| **Filter_HW_RgbdCamera** | 🔒 proprietary | local | Acquires images from an RGBD camera (Intel RealSense). |
| **Filter_HW_Imu** | 🔒 proprietary | local | Acquires inertial data from an IMU (Intel RealSense). |
| **Filter_Comm_DataChannel** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Comm_DataChannel/linux-gcc-x64-ubuntu-24/Filter_Comm_DataChannel.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> <a href="https://www.cybercortex.ai/data/filters/Filter_Comm_DataChannel/win-msvc-x64/Filter_Comm_DataChannel.zip" target="_blank">win-msvc-x64</a> | Communication between distributed DataBlocks (including CyberCortex.AI Droids). |
| **Filter_Visualization_Sensing** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Visualization_Sensing/linux-gcc-x64-ubuntu-24/Filter_Visualization_Sensing.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> <a href="https://www.cybercortex.ai/data/filters/Filter_Visualization_Sensing/win-msvc-x64/Filter_Visualization_Sensing.zip" target="_blank">win-msvc-x64</a> | Visualization of input and output filters results. |
