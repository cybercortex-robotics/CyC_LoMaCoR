# Real-time Localization and Mapping services for Collaborative aerial and ground Robots (LoMaCoR)

## Introduction

LoMaCoR is a localization and mapping service, which 1) enables a robot to access and retrieve SLAM maps stored in the EuroCore cloud, and 2) if a requested map area is not available, enable a robot to upload a new map on EuroCore, in order for other robots to access it via EuroCore.

## Quick start

### Dependencies ###

CyberCortex.AI dependencies:
```bash
sudo apt install libopencv-dev libfmt-dev libconfig++-dev liboctomap-dev libcurl4-openssl-dev libopenh264-dev qtbase5-dev freeglut3 libqglviewer2-qt5
```

LoMaCoR dependencies
```bash
sudo apt install libpsl-dev meson nlohmann-json3-dev 
```

Install the ```CPR``` library from <a href="https://github.com/libcpr/cpr" target="_blank">https://github.com/libcpr/cpr</a>

### Running the binaries

Download the binaries corresponding to you specific operating system from the Releases github section, or compile from source:

### Compile from source

Clone the repository, create the build folder and update the git submodules:

```bash
git clone https://github.com/cybercortex-robotics/CyC_LoMaCoR.git
cd CyC_LoMaCoR
mkdir build
```

While in the ```CyC_LoMaCoR``` folder, update the git submodule dependencies:
```bash
git submodule update --init --remote
```

Run ```cmake-gui``` and press Configure. Then activate the ```Build_App_CycCore```, ```Build_Filter_LoMaCoR_Maps``` and ```Build_Filter_LoMaCoR_Viz``` components:

<div align="center">
  <img src="https://github.com/cybercortex-robotics/CyC_LoMaCoR/blob/main/figures/cmake_filters_enable.png?raw=true" width="40%" alt="cmake_filters_enable" />
</div>

In ```cmake-gui``` press Configure, followed by Generate.

Go the the build folder and compile the project:
```bash
cd build
make -j$(nproc)
```

### Configure Zenodo access

Accessing Zenodo requires a url and an access token, both specified in configuration file ```etc/credentials.conf```

### Testing the LoMaCoR library

Use the test unit ```tu_Zenodo``` to manually access the Zenodo repository. The applications requires as input at least a region and te path to the credentials file.

Create a new region '''Brasov''':
```bash
tu_Zenodo --n Brasov ../etc/credentials.conf
```

Upload map ```2.zip``` to region ```Brasov```:
```bash
tu_Zenodo --r Brasov --u 2.zip ../etc/credentials.conf
```

List all maps in region ```Brasov```:
```bash
tu_Zenodo --r Brasov --l ../etc/credentials.conf
```

Download map ```1.zip``` from region ```Brasov```:
```bash
tu_Zenodo --r Brasov --d 1.zip ../etc/credentials.conf
```

### Python implementation

The Python implementation for accesing Zenodo is available in the ```resources/tu_Zenodo.py``` script. 

### Running LoMaCoR as CyberCortex.AI application

The LoMaCoR library can be run in the back ground using <a href="https://github.com/cybercortex-robotics/CyC_inference" target="_blank">CyberCortex.AI</a>. From the ```bin``` folder, run:
```bash
AppCycCore ../etc/datablocks/cyc.conf
```

```cyc.conf``` contains the configuration of the Lomacor filter:

```bash
Lomacor:
{
    ID              = 1
    Active          = True
    Type            = "CyC_LOMACOR_MAPS_FILTER_TYPE"
    IsPublishable   = True
    ReplayFromDB    = False
    dt              = 1.0
    dt_Sequencing   = 1.0
    InputSources    = ()
    Parameters      = (
                        {name = "zenodo_url", value = "https://sandbox.zenodo.org/api/deposit/depositions"},
                        {name = "access_token", value = "LP7zVxmRWR4WTRVWNn8pMEnyBNxyTpAEui3E2pKPMZ00tpGdtzDrLZB2dgFN"},
                        {name = "region", value = "Brasov"},
                        {name = "maps_folder", value = "etc/env/maps"},
                        {name = "map", value = "1"},
                        {name = "map_filetype", value = ".map"},
                        {name = "map_upload_th", value = "30"},
                        {name = "mapper", value = "true"}
                      )
}
```

The parameters are the Zenodo url and access token, the region's name, the maps folder, the name of the map to monitor and its extension, an upload threshold and a boolean value specifying if the robot is a mapper (maps the area and uploads the resulting map to Zenodo), or not (downloads a map from Zenodo).

The application is completly decoupled from the SLAM system by monitoring only the given map file. If the robot is a mapper, the map file is uploaded to Zenodo only if the map file how not been rewritten more than ```map_upload_th``` [sec]. If the robot queries the map from Zenodo (```mapper = false```), then LoMaCoR will download the map automaticaly once it is available on Zenodo.


## LoMaCoR in a complete CyberCortex.AI SLAM solution

LoMaCoR uses the following CyberCortex.AI filters. For details regarding the CyberCortex.AI OS, please visit <a href="https://www.cybercortex.ai" target="_blank">www.cybercortex.ai</a>.
For the proprietary filters, download their binaries and place them in the ```bin/filters``` folder.

| Component | License | Source | Notes |
| :--- | :---: | ----------: | :--- |
| **Filter_LoMaCoR_Maps** | 🌐 open-source | local | Implementation of the NavGraph communication protocol. |
| **Filter_LoMaCoR_Viz** | 🌐 open-source | local | LoMaCoR visualization. |
| **Filter_HW_RgbdCamera** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_HW_RgbdCamera/linux-gcc-x64-ubuntu-24/Filter_HW_RgbdCamera.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> win-msvc-x64 | Acquires images from an RGBD camera (Intel RealSense). |
| **Filter_HW_Imu** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_HW_Imu/linux-gcc-x64-ubuntu-24/Filter_HW_Imu.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> win-msvc-x64 | Acquires inertial data from an IMU (Intel RealSense). |
| **Filter_Comm_DataChannel** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Comm_DataChannel/linux-gcc-x64-ubuntu-24/Filter_Comm_DataChannel.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> <a href="https://www.cybercortex.ai/data/filters/Filter_Comm_DataChannel/win-msvc-x64/Filter_Comm_DataChannel.zip" target="_blank">win-msvc-x64</a> | Communication between distributed DataBlocks (including CyberCortex.AI Droids). |
| **Filter_Vision_VisualSlam** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Vision_VisualSlam/linux-gcc-x64-ubuntu-24/Filter_Vision_VisualSlam.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> win-msvc-x64 | Communication between distributed DataBlocks (including CyberCortex.AI Droids). |
| **Filter_Visualization_Sensing** | 🔒 proprietary | <a href="https://www.cybercortex.ai/data/filters/Filter_Visualization_Sensing/linux-gcc-x64-ubuntu-24/Filter_Visualization_Sensing.zip" target="_blank">linux-gcc-x64 (ubuntu 24)</a> <br> linux-gcc-arm-x64 <br> <a href="https://www.cybercortex.ai/data/filters/Filter_Visualization_Sensing/win-msvc-x64/Filter_Visualization_Sensing.zip" target="_blank">win-msvc-x64</a> | Visualization of input and output filters results. |


## Mapper configuration

The mapper robot can be configured using its configuration file ```etc/datablocks/mapper.conf```.
After starting the server, the console should print the sampling time of the server:

<div align="center">
  <img src="https://github.com/cybercortex-robotics/CyC_LoMaCoR/blob/main/figures/datablock_server.png?raw=true" width="60%" alt="cmake_filters_enable" />
</div>


## Map client configuration

The client on the edge (robot datablock) can be configured using its corresponding configuration file ```etc/datablocks/robot.conf```.

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
