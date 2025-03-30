#include "ModuleGroup.h"

// Default constructor
ModuleGroup::ModuleGroup() 
  : multiplexerAddress(0), moduleCount(0), 
    stepsPerRot(0), displayOffset(0), magnetPosition(0) {
}

// Constructor with multiplexer address
ModuleGroup::ModuleGroup(uint8_t multiplexerAddress, int stepsPerRot, int displayOffset, int magnetPosition)
  : multiplexerAddress(multiplexerAddress), moduleCount(0), 
    stepsPerRot(stepsPerRot), displayOffset(displayOffset), 
    magnetPosition(magnetPosition) {
}

// Select the TCA9548A multiplexer for this group (always using channel 0)
void ModuleGroup::selectMultiplexer() {
  Wire.beginTransmission(multiplexerAddress);
  Wire.write(1); // Select channel 0 (bit 0 set)
  Wire.endTransmission();
}

// Initialize the module group
void ModuleGroup::init() {
  // Select the multiplexer
  selectMultiplexer();
  
  // Initialize each module in the group
  for (int i = 0; i < moduleCount; i++) {
    modules[i].init();
  }
}

// Add a module to the group
bool ModuleGroup::addModule(uint8_t moduleAddress, int moduleOffset) {
  if (moduleCount >= MAX_MODULES_PER_GROUP) {
    return false; // Group is full
  }
  
  // Create a new module instance
  modules[moduleCount] = SplitFlapModule(
    moduleAddress, 
    stepsPerRot, 
    moduleOffset + displayOffset, 
    magnetPosition
  );
  
  // Store the module offset
  moduleOffsets[moduleCount] = moduleOffset;
  
  // Increment the module count
  moduleCount++;
  
  return true;
}

// Stop all modules in the group
void ModuleGroup::stopModules() {
  selectMultiplexer();
  for (int i = 0; i < moduleCount; i++) {
    modules[i].stop();
  }
}

// Start all modules in the group
void ModuleGroup::startModules() {
  selectMultiplexer();
  for (int i = 0; i < moduleCount; i++) {
    modules[i].start();
  }
}

// Get the target positions for all modules based on a single character
void ModuleGroup::getTargetPositions(char inputChar, int* targetPositions) {
  for (int i = 0; i < moduleCount; i++) {
    targetPositions[i] = modules[i].getCharPosition(inputChar);
  }
}

// Get the target positions for modules based on a string
void ModuleGroup::getTargetPositionsFromString(String inputString, int startIndex, bool centering, int* targetPositions) {
  String displayString;
  
  // Extract the portion of the string for this group
  if (startIndex < inputString.length()) {
    displayString = inputString.substring(startIndex, startIndex + moduleCount);
  } else {
    displayString = "";
  }
  
  // Apply centering if requested
  if (centering) {
    int totalPadding = moduleCount - displayString.length();
    int paddingLeft = totalPadding / 2;
    int paddingRight = totalPadding - paddingLeft;
    
    // Add padding to the left
    String result = "";
    for (int i = 0; i < paddingLeft; i++) {
      result += " ";
    }
    
    // Add the original string
    result += displayString;
    
    // Add padding to the right
    for (int i = 0; i < paddingRight; i++) {
      result += " ";
    }
    displayString = result;
  } else { // pad blanks to end, if no centering
    while (displayString.length() < moduleCount) {
      displayString += " "; // Padding with space
    }
  }
  
  // Iterate through the string and process each character
  for (int i = 0; i < displayString.length() && i < moduleCount; i++) {
    char currentChar = displayString[i];
    targetPositions[i] = modules[i].getCharPosition(currentChar);
  }
}

// Move all modules to target positions
void ModuleGroup::moveTo(int targetPositions[], float speed, bool releaseMotors) {
  selectMultiplexer();
  
  // Following the same pattern as SplitFlapDisplay::moveTo but for modules in this group
  speed = constrain(speed, 2, 15.0f); // Using 15.0f as MAX_RPM to match SplitFlapDisplay
  float stepsPerSecond = (speed / 60) * stepsPerRot;
  float timePerStep = 1000000 / stepsPerSecond;
  
  unsigned long currentTime = micros();
  
  int checkIntervalUs = 20 * 1000; // 20ms check interval for hall effect sensors
  int startStopDelay = 200; // time to wait for motor alignment
  
  bool resetLatches[MAX_MODULES_PER_GROUP] = {false};
  bool needsStepping[MAX_MODULES_PER_GROUP] = {false};
  unsigned long lastStepTimes[MAX_MODULES_PER_GROUP];
  unsigned long lastSensorCheckTime = currentTime;
  
  for (int i = 0; i < moduleCount; i++) {
    targetPositions[i] = constrain(targetPositions[i], 0, stepsPerRot - 1);
    resetLatches[i] = true;
    lastStepTimes[i] = currentTime;
    if (modules[i].getPosition() != targetPositions[i]) {
      needsStepping[i] = true;
    } else {
      needsStepping[i] = false;
    }
  }
  
  startModules();
  delay(startStopDelay);
  
  bool isFinished = true;
  for (int i = 0; i < moduleCount; i++) {
    if (needsStepping[i]) {
      isFinished = false;
      break;
    }
  }
  
  while (!isFinished) {
    currentTime = micros();
    for (int i = 0; i < moduleCount; i++) {
      if (((currentTime - lastStepTimes[i]) > timePerStep) && needsStepping[i]) {
        modules[i].step();
        lastStepTimes[i] = micros();
        if (modules[i].getPosition() == targetPositions[i]) {
          needsStepping[i] = false;
        }
      }
    }
    
    if ((currentTime - lastSensorCheckTime) > checkIntervalUs) {
      // Check hall effect sensors
      for (int i = 0; i < moduleCount; i++) {
        if (needsStepping[i] && (modules[i].readHallEffectSensor() == true)) {
          if (!resetLatches[i]) {
            modules[i].magnetDetected();
            resetLatches[i] = true;
          }
        } else if (resetLatches[i] == true) {
          resetLatches[i] = false;
        }
      }
      
      // Check if all modules are finished
      isFinished = true;
      for (int i = 0; i < moduleCount; i++) {
        if (needsStepping[i]) {
          isFinished = false;
          break;
        }
      }
      
      lastSensorCheckTime = currentTime;
    }
  }
  
  if (releaseMotors) {
    delay(startStopDelay);
    stopModules();
  }
}

// Home all modules in this group
void ModuleGroup::home(float speed) {
  selectMultiplexer();
  
  int targetPositions[MAX_MODULES_PER_GROUP];
  for (int i = 0; i < moduleCount; i++) {
    targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
  }
  
  startModules();
  moveTo(targetPositions, speed, false);
  
  char homeChar = ' ';
  for (int i = 0; i < moduleCount; i++) {
    targetPositions[i] = modules[i].getCharPosition(homeChar);
  }
  
  moveTo(targetPositions, speed);
}
