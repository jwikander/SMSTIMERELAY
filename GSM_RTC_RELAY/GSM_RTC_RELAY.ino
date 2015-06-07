#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>

// DEBUG //
#define DEBUG
//DEBUG //

// Settings
#define ON_HOUR_START 55
#define ON_HOUR_STOP 05

#define PIN_SIM900_RESET 6
#define PIN_SIM900_RX 7
#define PIN_SIM900_TX 8
#define PIN_SIM900_ON_OFF 9
#define PIN_COMPUTER_ON_OFF 10

// Time struct
struct Time
{
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
};

// Settings struct
struct Settings
{
  uint8_t minutesAroundSunUp;
  uint8_t hourWhenRecordingStart;
  uint8_t hourWhenRecordingStop;
};

// CONSTANTS
//
const String APN = "apn.telia.se";
const String APNUsername = "";
const String APNPassword = "";

// GLOBALS
//
SoftwareSerial SIM900(PIN_SIM900_RX, PIN_SIM900_TX); // RX, TX
Time time;
Settings settings;
bool isComputerOn = false; // True when computer is on
bool isManualModeOn = false; // True when computer is started manually, via SMS for example
String msg;

// Setup
void setup()
{
#ifdef DEBUG
  Serial.begin(19200);
#endif

  pinMode(PIN_SIM900_ON_OFF, OUTPUT); // Set the pin connected to Sim900 on/off pin as output
  pinMode(PIN_COMPUTER_ON_OFF, OUTPUT); // Set the pin connected to on/off relay as output

  EEPROM.get(0, settings); // Load saved settings from EEPROM at address 0 (beginning of memory)
    //    SIM900PowerOn(); // Start the SIM900 board and let it connect to the gsm network
    SIM900.begin(19200);
    SIM900.println("AT+CMGF=1");
    delay(200);
    //SIM900.println("ATE0"); // Disable command echo
    delay(50);
    SIM900.println("AT+CMGD=1,4"); // Clear the SMS storage on the SIM900


  readTime(); // Read time from ???
  // setRTC(); // Set current time to RTC
}

// Loop
void loop()
{
    readSMS();
    
    
    // Check the time to see if the computer should be started,
    // disabled if manual mode is set.
    if (!isManualModeOn)
    {
        readTime();
        
        if (time.minute == ON_HOUR_START && !isComputerOn)
        {
            sendComputerOnOffSignal();
            isComputerOn = true;
        }
    
        if (time.minute == ON_HOUR_STOP && isComputerOn)
        {
            sendComputerOnOffSignal();
            isComputerOn = false;
        }
    }



    // Only run this loop once every 10 seconds. This is for power saving idle mode.
    // TODO: Change delay to function with power saving mode.
    delay(10000);
}

void readSMS()
{
    // Always read the first SMS in the SMS storage,
    // the storage is always cleared after every read SMS
    // so all new SMS should be at location 1
    SIM900.println("AT+CMGR=1");
    delay(500);
    fillMessage();
}

// Send time request command to SIM900 and read answere.
void readTime()
{
    SIM900.println("AT+CCLK?");
    delay(500);
    fillMessage();
}

// This is called after a command is sent to the SIM900
// reads out information from the serial buffer
void fillMessage()
{
    char serialInByte;
    while (SIM900.available())
    {
        serialInByte = SIM900.read();
        delay(5);
        
        if(serialInByte == '\r')
        {
            // Commands ends with a <CR> so now we handle it
            handleMessage();
        }
        if(serialInByte == '\n')
        {
            //Skip Line Feed
        }
        else
        {
            msg += String(serialInByte);
        }
    }
}

// Read the message buffer and check for relevant information
void handleMessage()
{
    // Clock Message
    if (msg.indexOf("+CCLK") >= 0)
    {
        parseTime();    
    }
    
    // SMS Message
    if (msg.indexOf("+CMGR") >= 0)
    {
        // The contents of the SMS comes after the header which is ended by a <CR>
        // so clear the header with clearMessage() and continue read the contents.
        Serial.println("SMS Received!");
    }
    
    if (msg.indexOf("start") >= 0)
    {
        Serial.println("Start the damn computer!");
        SIM900.println("AT+CMGD=1,4"); // Clear the SMS storage on the SIM900
        enableManualMode();
    }
    
    if (msg.indexOf("stop") >= 0)
    {
        Serial.println("Stop the damn computer!");
        SIM900.println("AT+CMGD=1,4"); // Clear the SMS storage on the SIM900
        disableManualMode();
    }
    /*else
    {
        Serial.println(msg);
    }*/
        
        
    clearMessage();
}

// Clear the messge buffer
void clearMessage()
{
    msg = "";
}

// Parse time from a +CCLK message received from SIM900
void parseTime()
{
    time.hour = msg.substring(18, 20).toInt();
    time.minute = msg.substring(21, 23).toInt();
    time.second = msg.substring(24, 26).toInt();
    Serial.print("Time is: ");
    Serial.print(time.hour);
    Serial.print(":");
    Serial.print(time.minute);
    Serial.print(":");
    Serial.println(time.second);
}

// Parse SMS received from SIM900
void parseSMS()
{
    
}

// Enable manual mode
// if computer is off, turn it on
void enableManualMode()
{
    if (!isComputerOn)
    {
        sendComputerOnOffSignal();
        isComputerOn = true;
    }
    
    isManualModeOn = true;
}

// Disable manual mode
void disableManualMode()
{
    // Computer should always be on when this command is received
    // but just incase, check isComputerOn flag.
    if (isComputerOn)
    {
        sendComputerOnOffSignal();
        isComputerOn = false;
    }
    
    isManualModeOn = false;
}


// Set on/off relay high, wait two seconds and then set it low again.
// Should be more than enough time for computer to register on/off button press.
// TODO: Maybe change the delay to power optimized delay function. Not really needed as this is only run 2 times/hour.
void sendComputerOnOffSignal()
{
#ifdef DEBUG
    Serial.println("Setting computer on/off ..HIGH.. ");
#endif

    digitalWrite(PIN_COMPUTER_ON_OFF, HIGH);
    delay(1000);
    digitalWrite(PIN_COMPUTER_ON_OFF, LOW);

#ifdef DEBUG
    Serial.println("Setting computer on/off ..LOW.. ");
#endif
}

  void SIM900PowerOn()
  {
    digitalWrite(PIN_SIM900_ON_OFF, HIGH);
    delay(1000);
#ifdef DEBUG
    Serial.println("Setting Sim900 on/off ..LOW.. ");
#endif
    digitalWrite(PIN_SIM900_ON_OFF, LOW);
    delay(1000);
#ifdef DEBUG
    Serial.println("Setting Sim900 on/off ..HIGH.. ");
#endif
    digitalWrite(PIN_SIM900_ON_OFF, HIGH);
    delay(10000); // Wait a while to let the board log on to the gsm network
}
