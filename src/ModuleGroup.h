// ModuleGroup.h (header file)
#ifndef ModuleGroup_h
#define ModuleGroup_h

#include "SplitFlapModule.h"
#include <Arduino.h>
#include <Wire.h>

#define MAX_MODULES_PER_GROUP 8 // Maximum number of modules per multiplexer

class ModuleGroup {
public:
  // Default constructor
  ModuleGroup();
  
  // Constructor with multiplexer address
  ModuleGroup(uint8_t multiplexerAddress, int stepsPerRot, int displayOffset, int magnetPosition);
  
  // Initialize the module group
  void init();
  
  // Add a module to the group
  bool addModule(uint8_t moduleAddress, int moduleOffset);
  
  // Get module count
  int getModuleCount() const { return moduleCount; }
  
  // Step management functions
  void stopModules();
  void startModules();
  
  // Get the target positions for all modules based on input characters
  void getTargetPositions(char inputChar, int* targetPositions);
  void getTargetPositionsFromString(String inputString, int startIndex, bool centering, int* targetPositions);
  
  // Movement control - select multiplexer and move all modules
  void moveTo(int targetPositions[], float speed, bool releaseMotors = true);
  
  // Home all modules in this group
  void home(float speed);
  
  // Access modules directly if needed
  SplitFlapModule* getModules() { return modules; }
  
private:
  // Select the TCA9548A multiplexer for this group
  void selectMultiplexer();
  
  uint8_t multiplexerAddress;  // I2C address of the TCA9548A multiplexer
  int moduleCount;             // Number of modules in this group
  SplitFlapModule modules[MAX_MODULES_PER_GROUP]; // Array of modules
  
  int stepsPerRot;        // Number of steps per rotation (shared by all modules)
  int displayOffset;      // Global display offset
  int magnetPosition;     // Position where magnet is detected
  int moduleOffsets[MAX_MODULES_PER_GROUP]; // Individual module offsets
};

#endif
