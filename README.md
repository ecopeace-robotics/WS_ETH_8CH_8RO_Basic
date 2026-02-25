# Eth_Motor
esp32-s3 릴레이보드로 mdds30 모터드라이버 제어, dc모터 2개 제어

## H/W
### MCU Board
#### Waveshare ESP32-S3-ETH-8DI-8RO

### Motor Driver
#### MDDS30
- [사용법](./Docs/MDDS30_Howto.md)


## S/W

### Motor Driver Library
[Arduino Library Link](https://github.com/CytronTechnologies/CytronMotorDriver?tab=readme-ov-file)

## Example folder contents

The project **sample_project** contains one source file in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
