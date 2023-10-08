#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

//#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0])) //simply finds the number of elements in an array

typedef struct {        //creates a struct that is essentially a template of each channel and its related data
  char cName;
  int num;
  String description;
  int maximum = 255;
  int minimum = 0;
} channel;

byte upArrow[] = { B00100, B01110, B10101, B00100, B00000, B00000, B00000, B00000 };  //defining characters for the UDCHARS extension
byte downArrow[] = { B00000, B00000, B00000, B00000, B00100, B10101, B01110, B00100 };

channel channelArray[26]; //creates an array of type "channel" which will store all the possible channels
int numArray[64];         //creates an array which will store the most recent 64 numbers entered

char channelElemTop;//contains the channel name for the top row of the lcd
char channelElemBot;//contains the name for the bottom row

unsigned long previousMillis = 0;//for the SCROLL extension

int findIndex(char value) {//finds the index of a channel based on the channel name's ascii value
  int index = value - 65;
  return index;
}

void setup() {
  lcd.begin(16, 2);
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);
  Serial.begin(9600);
  //below is the synchronisation
  lcd.setBacklight(5);
  while (Serial.read() != 'X') {
    Serial.print("Q");
    delay(1000);
  }
  Serial.println("RECENT, FREERAM, SCROLL, UDCHARS, NAMES");
  lcd.setBacklight(7);
  delay(1000);
  lcd.clear();

}

String numJustify(int num) {//takes a number and returns a right-justified string of it
  String numString = String(num);
  if (num < 100) {
    numString = ' ' + numString;
  }
  if (num >= 0 && num < 10) {
    numString = ' ' + numString;
  }
  return numString;
}

void primary(char channelElemTop) {             //function that controls the display of the top row of the lcd
  String row;
  int index = findIndex(channelElemTop);
  if (index == findFirst()) {//if the channel displayed is currently the first channel alphabetically, displays without the custom arrow
    clearPrimary();
    row = String(channelElemTop) + numJustify(channelArray[index].num) + "," + String(getAverage()) + "," + channelArray[index].description;
    lcd.setCursor(1, 0);
    if (row.length() <= 16) {
      lcd.print(row);
    }
    else {//if the message is longer than 16 characters then scroll
      scroll(row, 0);
    }
  }
  else {//if the channel displayed is not the first alphabetically, displays with the custom arrow
    lcd.setCursor(0, 0);
    lcd.write(0);
    lcd.setCursor(1, 0);
    row = String(channelElemTop) + numJustify(channelArray[index].num) + "," + String(getAverage()) + "," + channelArray[index].description;
    if (row.length() <= 16) {
      lcd.print(row);
    }
    else {
      scroll(row, 0);
    }
  }
}

void secondary(char channelElemBot) {
  String row;
  int index = findIndex(channelElemBot);
  if (index == findLast()) {
    clearSecondary();
    lcd.setCursor(1, 1);
    row = String(channelElemBot) + numJustify(channelArray[index].num) + "," + String(getAverage()) + "," + channelArray[index].description;
    if (row.length() <= 16) {
      lcd.print(row);
    }
    else {
      scroll(row, 1);
    }
  }
  else if (index < findLast()) {
    lcd.setCursor(0, 1);
    lcd.write(1);
    lcd.setCursor(1, 1);
    row = String(channelElemBot) + numJustify(channelArray[index].num) + "," + String(getAverage()) + "," + channelArray[index].description;
    if (row.length() <= 16) {
      lcd.print(row);
    }
    else {
      scroll(row, 1);
    }
  }
}

void scroll(String message, int pos) {                           //takes the message to display and the y position of the message as parameters
  for (int letter = 0; letter <= message.length() - 15; letter += 2)            //scrolls left
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= 100) {                                   //every 100ms it scrolls 2 characters left
      lcd.setCursor(1  , pos);
      for (int currentLetter = letter; currentLetter < message.length(); currentLetter++)
      {
        lcd.print(message[currentLetter]);
      }
      lcd.print(" ");
      previousMillis = currentMillis;

    }
  }
}


void clearPrimary() {//functions to clear the top and bottom row respectively
  lcd.setCursor(0, 0);
  lcd.print("                ");
}
void clearSecondary() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

//various functions ( FOR RECENT EXTENSION)
int totalElemNumArray() {         //finds the total amount of elements that occupy numArray
  int count = 0;
  for (int i = 0; i <= 63; i++) {
    if (numArray[i] == 0) {         //by default, each 64 elements contain 0, so the program counts how many 0's there are
      count++;
    }
  }
  return 63 - count;
}
void addToList(int num) {
  if (num == 13 || num == 10)  {}           //do nothing - ignore CR and NL (here for testing purposes                                       
  else {
    if (totalElemNumArray() == 63) {        //if every index of the num array has been written to, then the list is shifted left to make space for the new element
      for (int i = 0; i <= 62; i++) {
        numArray[i] = numArray[i + 1];
      }
      numArray[63] = num;
    }
    else if (totalElemNumArray() < 63) {
      for (int i = 0; i <= 62; i++) {
        if (numArray[i] == 0) {           //finds the first place in the list which hasn't been written to and inserts the number there
          numArray[i] = num;
          break;
        }
      }
    }
  }
}

int getAverage() {
  int sum = 0;
  int count = 0;
  for (int i = 0; i <= 63; i++) {//goes through the list and sums the numbers, but only counts numbers that have been written to
    sum += numArray[i];
    if (numArray[i] != 0) {
      count += 1;
    }
  }
  int average = sum / count;
  return round(average);
}
//


channel cycleUp(char channelElemTop) {                            //dictates what happens when up is pressed
  int index = findIndex(channelElemTop);
  channel channelInstance;                                        //creates an instance of the channel structure so that I can return values of different types
  if (index >= findFirst()) {                                     //if the current channel on the top row of the lcd is not the first alphabetically then it does the following
    for (int i = index - 1; i >= 0; i--) {
      if (channelArray[i].cName != 0) {
        channelInstance.cName = channelArray[i].cName;
        channelInstance.num = channelArray[i].num;
        channelInstance.description = channelArray[i].description;
        return channelInstance;
      }
    }
  }
  else {//if the current channel is the first alphabetically, then it doesn't return anything new
    channelInstance.cName = channelArray[index].cName;
    channelInstance.num = channelArray[index].num;
    channelInstance.description = channelArray[index].description;
    return channelInstance;
  }
}

channel cycleDown(char channelElemBot) {//dictates what happens when down is pressed
  int index = findIndex(channelElemBot);
  channel channelInstance;
  if (index < findLast()) {
    for (int i = index + 1; i <= findLast(); i++) {
      if (channelArray[i].cName != 0) {
        channelInstance.cName = channelArray[i].cName;
        channelInstance.num = channelArray[i].num;
        channelInstance.description = channelArray[i].description;
        return channelInstance;

      }
    }
  }
  else {
    channelInstance.cName = channelArray[index].cName;
    channelInstance.num = channelArray[index].num;
    channelInstance.description = channelArray[index].description;
    return channelInstance;
  }
}

int freeRam() {                     //finds the amount of free SRAM (taken from lab3 worksheet)
  extern char *__brkval;
  char top;
  return (int)&top - (int)__brkval;
}


void selectAction() {                     //dictates what happens when the select button is pressed
  lcd.clear();
  lcd.setBacklight(5);
  lcd.setCursor(0, 0);
  lcd.print("F121572");
  lcd.setCursor(0, 1);
  lcd.print("SRAM:" + String(freeRam()));
}

void createChannel(char inputName, String inputDescription) {       //a channel is created based on the alphabetical order of the channel name
  int index = inputName;
  if (channelArray[index - 65].cName == 0) {                      //if the struct at the alphabetical position of the channel name is empty:
    channelArray[index - 65].cName = inputName;
    channelArray[index - 65].num = 0;
    channelArray[index - 65].description = inputDescription;
    channelArray[index - 65].minimum = 0;
    channelArray[index - 65].maximum = 255;
  }
  else {                                                      //if it is not empty then it simply updates the channel description
    channelArray[index - 65].cName = inputName;
    channelArray[index - 65].description = inputDescription;
  }
}

//functions to update the struct for each channel
void updateValue(char inputName, int inputValue) {
  int index = inputName;
  channelArray[index - 65].num = inputValue;
}

void updateMax(char inputName, int inputMax) {
  int index = inputName;
  channelArray[index - 65].maximum = inputMax;

}
void updateMin(char inputName, int inputMin) {
  int index = inputName;
  channelArray[index - 65].minimum = inputMin;

}

void createAndUpdateArray() {//dictates what happens when things are input to the serial monitor
  while (Serial.available() > 0) {
    String input = Serial.readString();
    if (input.charAt(0) == 'C') {
      char inputName = input.charAt(1);
      String inputDescription = input.substring(2, input.indexOf('\n'));
      createChannel(inputName, inputDescription);

    }
    else if (input.charAt(0) == 'V') {
      char inputName = input.charAt(1);
      String inputNumString = input.substring(2, input.indexOf('\n'));
      int inputNum = inputNumString.toInt();
      addToList(inputNum);
      updateValue(inputName, inputNum);
    }
    else if (input.charAt(0) == 'X') {
      char inputName = input.charAt(1);
      String inputMaxString = input.substring(2, input.indexOf('\n'));
      int inputMax = inputMaxString.toInt();
      updateMax(inputName, inputMax);
    }
    else if (input.charAt(0) == 'N') {
      char inputName = input.charAt(1);
      String inputMinString = input.substring(2, input.indexOf('\n'));
      int inputMin = inputMinString.toInt();
      updateMin(inputName, inputMin);
    }
  }
}

int totalElemChannelArray() {//finds the total non-empty elements of channelArray
  int count = 0;
  for (int i = 0; i < 25; i++) {
    if (channelArray[i].cName == 0) {
      count++;
    }
  }
  return 25 - count;
}

int findFirst() {//finds the index of the first non-empty element of the channelArray
  for (int i = 0; i < 25; i++) {
    if (channelArray[i].cName != 0) {
      return i;
    }
  }
}
int findLast() {
  for (int i = 25; i > 0; i--) {
    if (channelArray[i].cName != 0) {
      return i;
    }
  }

}

void backlightChange() {//dictates what happens to the backlight depending on the range of the values
  bool minFlag = false;
  bool maxFlag = false;
  for (int i = 0; i < 25; i++) {
    if (channelArray[i].minimum < channelArray[i].maximum) {
      if (channelArray[i].num < channelArray[i].minimum) {
        minFlag = true;
        lcd.setBacklight(2);
      }
      else if (channelArray[i].num > channelArray[i].maximum) {
        maxFlag = true;
        lcd.setBacklight(1);
      }
    }
  }
  if (minFlag == false && maxFlag == false) {//if all numbers are within the max and min
    lcd.setBacklight(7);
  }
  else if (minFlag == true && maxFlag == true) {//if there exist numbers lower than the min and higher than the max
    lcd.setBacklight(3);
  }
  else if (minFlag == true && maxFlag == true) {//if there only exist numbers below the min
    lcd.setBacklight(2);
  }
  else if (minFlag == false && maxFlag == true) {//if there only exist numbers above the max
    lcd.setBacklight(1);
  }

}



enum states { INITIALISATION, WAITING_PRESS, WAITING_RELEASE };

void loop() {

  //static int num;
  static enum states state = INITIALISATION;
  static int button_pressed; // Store which button was pressed
  static long press_time; // Store the time the button was pressed
  static int last_b = 0;
  int b;
  static int pressed;
  static int released;

  //  primary(channelElemTop);
  //  secondary(channelElemBot);


  switch (state) {
    case INITIALISATION:
      createAndUpdateArray();                             //takes input from the serial monitor
      if (totalElemChannelArray() >= 2) {                 //if the number of available channels is 2, then it goes to the next state
        channelElemTop = channelArray[findFirst()].cName;
        channelElemBot = channelArray[findLast()].cName;
        state = WAITING_PRESS;
      }
      else if (totalElemChannelArray() == 1) {
        channelElemTop = channelArray[findFirst()].cName;
        primary(channelElemTop);
      }
      break;

    case WAITING_PRESS:
      backlightChange();
      createAndUpdateArray();
      primary(channelElemTop);    //displays the top row repeatedly until the next state
      secondary(channelElemBot);

      b = lcd.readButtons();
      pressed = b & ~last_b;
      last_b = b;

      if (b & (BUTTON_UP | BUTTON_DOWN | BUTTON_SELECT)) {
        if (b & BUTTON_UP) {
          channel ch = cycleUp(channelElemTop);
          if (channelElemTop != channelArray[findFirst()].cName) {
            channelElemBot = channelElemTop;
            channelElemTop = ch.cName;
          }
        }
        else if (b & BUTTON_DOWN) {
          channel ch = cycleDown(channelElemBot);
          if (channelElemBot != channelArray[findLast()].cName) {
            channelElemTop = channelElemBot;
            channelElemBot = ch.cName;
          }
        }
        button_pressed = pressed;
        press_time = millis();
        state = WAITING_RELEASE;
      }
      break;

    case WAITING_RELEASE:
      if (millis() - press_time >= 1000) {
        press_time = millis();
        if (button_pressed == BUTTON_SELECT) {        //if select is held for more than 1000 ms then it does the following
          selectAction();
        }
      }
      else {                                    //else it sets everything back to normal, ready for the WAITING_PRESS state
        b = lcd.readButtons();
        released = ~b & last_b;
        last_b = b;
        if (released & button_pressed) {
          lcd.clear();
          lcd.setBacklight(7);
          state = WAITING_PRESS;
        }
      }
  }
}
