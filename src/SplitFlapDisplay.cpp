#include "SplitFlapDisplay.h"
#include "JsonSettings.h"
#include "ModuleGroup.h"
#include "SplitFlapModule.h"

SplitFlapDisplay::SplitFlapDisplay(JsonSettings &settings)
    : settings(settings), numModules(0), numGroups(0), usingMultiplexers(false) {}

    void SplitFlapDisplay::init() {
      // Load common settings
      stepsPerRot = settings.getInt("stepsPerRot");
      displayOffset = settings.getInt("displayOffset");
      magnetPosition = settings.getInt("magnetPosition");
      maxVel = settings.getFloat("maxVel");
      
      SDAPin = settings.getInt("sdaPin");
      SCLPin = settings.getInt("sclPin");
      
      // Initialize I2C bus
      Wire.begin(SDAPin, SCLPin);
      Wire.setClock(400000);  // Set I2C clock speed to 400kHz
    
      // Load module addresses and offsets
      std::vector<int> settingAddresses = settings.getIntVector("moduleAddresses");
      std::vector<int> settingOffsets = settings.getIntVector("moduleOffsets");
      
      // Store module configuration - derive modulesPerGroup from moduleAddresses size
      modulesPerGroup = settingAddresses.size();
      if (modulesPerGroup > MAX_MODULES) {
        modulesPerGroup = MAX_MODULES;
        Serial.println("Warning: Too many module addresses defined. Limited to " + String(MAX_MODULES));
      }
      
      // Store the module addresses and offsets
      for (int i = 0; i < modulesPerGroup; i++) {
        moduleAddresses[i] = (uint8_t)settingAddresses[i];
        moduleOffsets[i] = settingOffsets[i];
      }
    
      // Check if multiplexerAddresses setting exists
      try {
        std::vector<int> settingMultiplexers = settings.getIntVector("multiplexerAddresses");
        usingMultiplexers = (settingMultiplexers.size() > 0);
        
        if (usingMultiplexers) {
          // Initialize with multiplexers
          numGroups = settingMultiplexers.size();
          if (numGroups > MAX_GROUPS) {
            numGroups = MAX_GROUPS;  // Limit to max groups
            Serial.println("Warning: Too many multiplexers defined. Limited to " + String(MAX_GROUPS));
          }
          
          // Calculate total number of modules
          numModules = numGroups * modulesPerGroup;
          Serial.println("Initializing with " + String(numGroups) + " module groups, " +
                        String(modulesPerGroup) + " modules per group, " +
                        String(numModules) + " total modules");
          
          // Store multiplexer addresses
          for (int i = 0; i < numGroups; i++) {
            multiplexerAddresses[i] = (uint8_t)settingMultiplexers[i];
          }
          
          // Print debugging info
          Serial.print("Module Offsets: ");
          for (int i = 0; i < modulesPerGroup; i++) {
            Serial.print(moduleOffsets[i]);
            Serial.print(" ");
          }
          Serial.println();
          
          // Initialize module groups
          for (int i = 0; i < numGroups; i++) {
            // Create a new module group
            groups[i] = ModuleGroup(
              multiplexerAddresses[i],  
              stepsPerRot,              
              displayOffset,            
              magnetPosition            
            );
            
            // Add modules to the group - all groups have the same number of modules
            for (int j = 0; j < modulesPerGroup; j++) {
              groups[i].addModule(moduleAddresses[j], moduleOffsets[j]);
            }
            
            // Initialize the group
            groups[i].init();
          }
        } else {
          // Fall back to direct connection
          numModules = modulesPerGroup;
          initializeDirectConnection();
        }
      } catch (const std::runtime_error&) {
        // If the multiplexerAddresses key doesn't exist, use direct connection
        usingMultiplexers = false;
        numModules = modulesPerGroup;
        initializeDirectConnection();
      }
    }

// Initialize modules with direct connection (original code)
void SplitFlapDisplay::initializeDirectConnection() {
  Serial.println("Initializing with direct connection");
  
  // Debug output
  Serial.println("Module Offsets: ");
  for (int i = 0; i < numModules; i++) {
    Serial.print(moduleOffsets[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Create module instances
  for (uint8_t i = 0; i < numModules; i++) {
    modules[i] = SplitFlapModule(
        moduleAddresses[i], 
        stepsPerRot,
        moduleOffsets[i] + displayOffset, 
        magnetPosition
    );
  }

  // Initialize each module
  for (uint8_t i = 0; i < numModules; i++) {
    modules[i].init();
  }
}

void SplitFlapDisplay::testAll() {
  char testChars[37] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                        'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
                        'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2',
                        '3', '4', '5', '6', '7', '8', '9'};
  int numChars = sizeof(testChars) / sizeof(testChars[0]);
  int targetPositions[numModules];

  int charPos;
  for (int i = 0; i < numChars; i++) {
    if (usingMultiplexers) {
      writeChar(testChars[i]);
    } else {
      for (int j = 0; j < numModules; j++) {
        targetPositions[j] = modules[j].getCharPosition(testChars[i]);
      }
      moveTo(targetPositions);
    }
    delay(500);
  }
}

void SplitFlapDisplay::testRandom(float speed) {
  char testChars[37] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                        'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
                        'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2',
                        '3', '4', '5', '6', '7', '8', '9'};

  int targetPositions[numModules];
  char randChar;

  Serial.print("Target: ");
  
  if (usingMultiplexers) {
    String randomString = "";
    for (int i = 0; i < numModules; i++) {
      randChar = testChars[random(0, 37)];
      randomString += randChar;
      Serial.print(randChar);
    }
    Serial.println(" ");
    writeString(randomString, speed, false);
  } else {
    for (int i = 0; i < numModules; i++) {
      randChar = testChars[random(0, 37)];
      targetPositions[i] = modules[i].getCharPosition(randChar);
      Serial.print(randChar);
    }
    Serial.println(" ");
    moveTo(targetPositions, speed);
  }
}

void SplitFlapDisplay::testCount() {
  int count = 0;
  int maxCount = pow(10, numModules);
  char targetChar;
  int targetInteger;

  int targetPositions[numModules];

  for (int i = 0; i < maxCount; i++) {
    String countString = "";
    
    if (usingMultiplexers) {
      // Create a string representation of the count
      int tempCount = i;
      for (int j = numModules - 1; j >= 0; j--) {
        targetInteger = (tempCount / (int)pow(10, j)) % 10;
        countString = String(targetInteger) + countString;
      }
      // Pad with zeros if needed
      while (countString.length() < numModules) {
        countString = "0" + countString;
      }
      writeString(countString, MAX_RPM, false);
    } else {
      // Original code
      for (int j = 0; j < numModules; j++) {
        targetInteger = (i % (int)pow(10, j + 1)) / (int)pow(10, j);
        targetChar = targetInteger + '0'; // convert to char
        targetPositions[numModules - j - 1] = modules[j].getCharPosition(targetChar);
      }
      moveTo(targetPositions);
    }
    delay(250);
  }
}

void SplitFlapDisplay::home(float speed) {
  Serial.println("Homing");
  
  if (usingMultiplexers) {
    for (int i = 0; i < numGroups; i++) {
      groups[i].home(speed);
    }
  } else {
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
      targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false);
    char homeChar = ' ';
    for (int i = 0; i < numModules; i++) {
      targetPositions[i] = modules[i].getCharPosition(homeChar);
    }
    moveTo(targetPositions, speed);
  }
}

void SplitFlapDisplay::homeToString(String homeString, float speed, bool centering) {
  Serial.println("Homing");
  
  if (usingMultiplexers) {
    for (int i = 0; i < numGroups; i++) {
      groups[i].home(speed);
    }
    writeString(homeString, speed, centering);
  } else {
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
      targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false);
    writeString(homeString, speed, centering);
  }
}

void SplitFlapDisplay::homeToChar(char homeChar, float speed) {
  Serial.println("Homing");
  
  if (usingMultiplexers) {
    for (int i = 0; i < numGroups; i++) {
      groups[i].home(speed);
    }
    writeChar(homeChar, speed);
  } else {
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
      targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false);

    for (int i = 0; i < numModules; i++) {
      targetPositions[i] = modules[i].getCharPosition(homeChar);
    }
    moveTo(targetPositions, true, speed);
  }
}

void SplitFlapDisplay::writeChar(char inputChar, float speed) {
  if (usingMultiplexers) {
    // Send the character to all module groups
    for (int i = 0; i < numGroups; i++) {
      int groupTargets[MAX_MODULES_PER_GROUP];
      groups[i].getTargetPositions(inputChar, groupTargets);
      groups[i].moveTo(groupTargets, speed);
    }
  } else {
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
      targetPositions[i] = modules[i].getCharPosition(inputChar);
    }
    moveTo(targetPositions, speed);
  }
}

void SplitFlapDisplay::writeString(String inputString, float speed, bool centering) {
  // If string is longer than number of modules, truncate it
  String displayString = inputString.substring(0, numModules);

  if (usingMultiplexers) {
    if (centering) {
      // Calculate global padding for the entire display
      int totalPadding = numModules - displayString.length();
      int paddingLeft = totalPadding / 2;
      
      // Process each group
      int modulesSoFar = 0;
      for (int i = 0; i < numGroups; i++) {
        int groupSize = groups[i].getModuleCount();
        int groupTargets[MAX_MODULES_PER_GROUP];
        
        // Get the offset into the padded string for this group
        int stringStartIndex = modulesSoFar - paddingLeft;
        if (stringStartIndex < 0) stringStartIndex = 0;
        
        // Get target positions for this group
        groups[i].getTargetPositionsFromString(displayString, stringStartIndex, false, groupTargets);
        
        // Move this group
        groups[i].moveTo(groupTargets, speed);
        
        modulesSoFar += groupSize;
      }
    } else {
      // Process each group sequentially without centering
      int modulesSoFar = 0;
      for (int i = 0; i < numGroups; i++) {
        int groupSize = groups[i].getModuleCount();
        int groupTargets[MAX_MODULES_PER_GROUP];
        
        // Get target positions for this group
        groups[i].getTargetPositionsFromString(displayString, modulesSoFar, false, groupTargets);
        
        // Move this group
        groups[i].moveTo(groupTargets, speed);
        
        modulesSoFar += groupSize;
      }
    }
  } else {
    // Original code
    if (centering) {
      int totalPadding = numModules - displayString.length();
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
      while (displayString.length() < numModules) { // Pad with spaces
        displayString += " ";                       // Padding with space
      }
    }

    int targetPositions[numModules];
    // Iterate through the input string and process each character
    for (int i = 0; i < displayString.length(); i++) {
      char currentChar = displayString[i];
      targetPositions[i] = modules[i].getCharPosition(currentChar);
    }
    moveTo(targetPositions, speed);
  }
}

void SplitFlapDisplay::moveTo(int targetPositions[], float speed, bool releaseMotors) {
  if (usingMultiplexers) {
    // Split the target positions by group and call moveTo on each group
    int positionIndex = 0;
    
    for (int i = 0; i < numGroups; i++) {
      int groupSize = groups[i].getModuleCount();
      int groupTargets[MAX_MODULES_PER_GROUP];
      
      // Extract the positions for this group
      for (int j = 0; j < groupSize; j++) {
        groupTargets[j] = targetPositions[positionIndex++];
      }
      
      // Move the modules in this group
      groups[i].moveTo(groupTargets, speed, releaseMotors);
    }
  } else {
    // Original code for direct connection
    speed = constrain(speed, 2, maxVel);
    float stepsPerSecond = (speed / 60) * stepsPerRot;
    float timePerStep = 1000000 / stepsPerSecond;

    unsigned long currentTime = micros();

    int checkIntervalUs = 20 * 1000; // How often to check each modules hall effect sensor
    int startStopDelay = 200; // time to wait to let motor realign itself to magnetic field

    bool resetLatches[numModules] = {}; // Initialize to false
    bool needsStepping[numModules] = {}; // Initialize to false;
    unsigned long lastStepTimes[numModules] = {}; // Initialize to false;
    unsigned long lastSensorCheckTime = currentTime;

    for (int i = 0; i < numModules; i++) {
      targetPositions[i] = constrain(targetPositions[i], 0, stepsPerRot - 1);
      resetLatches[i] = true;
      lastStepTimes[i] = currentTime;
      if (modules[i].getPosition() != targetPositions[i]) {
        needsStepping[i] = true;
      } else {
        needsStepping[i] = false;
      }
    }

    startMotors();
    delay(startStopDelay);

    bool isFinished = checkAllFalse(needsStepping, numModules);
    while (!isFinished) {
      currentTime = micros();
      for (int i = 0; i < numModules; i++) {
        if (((currentTime - lastStepTimes[i]) > timePerStep) && needsStepping[i]) {
          modules[i].step();
          lastStepTimes[i] = micros();
          if (modules[i].getPosition() == targetPositions[i]) {
            needsStepping[i] = false;
          }
        }
      }

      if ((currentTime - lastSensorCheckTime) > checkIntervalUs) {
        // check hall effect sensor every checkIntervalMs
        // check every modules sensor
        for (int i = 0; i < numModules; i++) {
          if (needsStepping[i] && (modules[i].readHallEffectSensor() == true)) {
            if (!resetLatches[i]) {
              modules[i].magnetDetected();
              resetLatches[i] = true;
            }
          } else if (resetLatches[i] == true) {
            resetLatches[i] = false;
          }
        }
        isFinished = checkAllFalse(needsStepping, numModules);
        lastSensorCheckTime = currentTime;
      }
    }
    
    if (releaseMotors) {
      delay(startStopDelay);
      stopMotors();
    }
  }