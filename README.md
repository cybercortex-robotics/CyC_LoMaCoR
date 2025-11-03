# Real-time Localization and Mapping services for Collaborative aerial and ground Robots (LoMaCoR)

## Introduction

LoMaCoR uses the following CyberCortex.AI filters. For details regarding the CyberCortex.AI OS, please visit <a href="https://www.cybercortex.ai" target="_blank">www.cybercortex.ai</a>.
For the proprietary filters, download their binaries and place them in the ```bin/filters``` folder.

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

Configure the project in the cmake-gui utility. Enable the three local filters and the application inference core:

<div align="center">
  <img src="https://github.com/cybercortex-robotics/CyC_LoMaCoR/blob/main/figures/cmake_filters_enable.png?raw=true" width="40%" alt="cmake_filters_enable" />
</div>

## Map server configuration

The server can be configured using its configuration file ```etc/datablocks/server.conf```.
After starting the server, the console should print the sampling time of the server:

<div align="center">
  <img src="https://github.com/cybercortex-robotics/CyC_LoMaCoR/blob/main/figures/datablock_server.png?raw=true" width="60%" alt="cmake_filters_enable" />
</div>


## Map client configuration

The client on the edge (robot datablock) can be configured using its corresponding configuration file ```etc/datablocks/robot_01.conf```.

For running the Filter_Vision_VisualSlam algorithm on the robot, download the corresponding Bag of Words vocabulary file:

| BoW implementation | Source |
| :--- | :---: |
| **DBoW2** | <a href="https://www.cybercortex.ai/data/filters/Filter_Vision_VisualSlam/vocab/ORBvoc_DBoW2.zip" target="_blank">ORBvoc_DBoW2</a> |
| **FBoW** (Intel processors only) | <a href="https://www.cybercortex.ai/data/filters/Filter_Vision_VisualSlam/vocab/ORBvoc_FBoW.zip" target="_blank">ORBvoc_FBoW</a> |

Provide the path to the downloaded vocabulary file in the filter's configuration area:
```bash
{name = "vocab", value = "etc/env/ORBvoc.txt"},
```

Download the default map file <a href="https://www.cybercortex.ai/data/filters/Filter_Vision_VisualSlam/maps/exploratory.zip" target="_blank">exploratory.zip</a> and provide its path in the filter's configuration area:
```bash
{name = "map", value = "etc/env/maps/exploratory.map"},
```

After starting the robot's datablock, the console should print the state of the datablock:

<div align="center">
  <img src="https://github.com/cybercortex-robotics/CyC_LoMaCoR/blob/main/figures/datablock_robot.png?raw=true" width="60%" alt="cmake_filters_enable" />
</div>

## Citation

Please cite our work if you enjoy CyberCortex.AI:

```
@article{CyberCortex_AI,
  author = {Grigorescu, Sorin and Zaha, Mihai},
  title = {CyberCortex.AI: An AI-based operating system for autonomous robotics and complex automation},
  journal = {Journal of Field Robotics},
  volume = {42},
  number = {2},
  pages = {474-492},
  doi = {https://doi.org/10.1002/rob.22426},
  url = {https://onlinelibrary.wiley.com/doi/abs/10.1002/rob.22426},
  year = {2025}
}
```
