/*
 * Arduino Mega - 5101 RAM Tester
 * 
 * pierre@fabriqueurs.com May 2022
 * 
 */

//                                  Wiring
//       Arduino Mega      <->       RAM  5101            
//  pin num -  pin type    <->  pin num -  pin name
//     26      Digital              4         A0
//     47      Digital              3         A1
//     48      Digital              2         A2
//     49      Digital              1         A3
//     50      Digital              21        A4
//     51      Digital              5         A5
//     52      Digital              6         A6
//     53      Digital              7         A7
//     43      Digital              18        OD
//     42      Digital              20        R/W
//     30      Digital              9         DI1
//     31      Digital              11        DI2
//     32      Digital              13        DI3
//     33      Digital              15        DI4
//     34      Digital              10        DO1
//     35      Digital              12        DO2
//     36      Digital              14        DO3
//     37      Digital              16        DO4     
//     GND     GND                  8         GND
//     GND     GND                  19        _CE1
//     5V      5V                   22        VCC
//     5V      5V                   17        CE2

#define pinCS1 24 // _CE1 on 5101 - not connected on UNO so use GND as this is active LOW
#define pinCS2 25 //  CE2 on 5101 - not connected on UNO so use +5V as this is active HIGH

// Address bus
#define pinA0 26
#define pinA1 47
#define pinA2 48
#define pinA3 49
#define pinA4 50
#define pinA5 51
#define pinA6 52
#define pinA7 53


#define pinOE 43 // OD pin on 5201
#define pinWE 42 // R/W pin on 5101

// Data bus (In - DIx)
#define pinD0 32
#define pinD1 33
#define pinD2 34
#define pinD3 35

// Data bus (Out - DOx)
#define pinO0 36
#define pinO1 37
#define pinO2 38 
#define pinO3 39 

#define max_addr 0xff

#define SERIAL_BAUD_RATE 9600

uint8_t addr_pins[] = {pinA0, pinA1, pinA2, pinA3, pinA4, pinA5, pinA6, pinA7};
uint8_t out_pins[] = {pinD0, pinD1, pinD2, pinD3};
uint8_t in_pins[] = {pinO0, pinO1, pinO2, pinO3};

bool debug = false; //debug mode (detailed serial output)

int failed = 0;




//----------------------------------------------------------
// Low level functions (set address, read and write data)
//----------------------------------------------------------
void setAddress(uint8_t address)
{        
    for (int i = 0; i < 8; i++) 
    {
       digitalWrite(addr_pins[i], ((address >> i) & 1));    
    }
    delay(1);
    if (debug) {
        Serial.println("Address set: " + String(address, BIN));
    }
}

void writeData(uint8_t data)
{
    //write enable start
    digitalWrite(pinOE, HIGH);
    delay(1);
    digitalWrite(pinWE, LOW);
    delay(1);
    //ready to write
    
    for (int i = 0; i < sizeof(out_pins); i++) 
    {
      digitalWrite(out_pins[i], ((data >> i) & 1));
    }
    delay(1);
    digitalWrite(pinWE, HIGH);
    // write completed
    if (debug) {
        Serial.println("Write data: " + String(data, BIN));
    }
}

int readData()
{
    //read enable start
    digitalWrite(pinWE, HIGH);
    delay(1);
    digitalWrite(pinOE, LOW);
    delay(1);
    //read enable end
    
    uint8_t readback=0;
    for (int i = 0; i < sizeof(in_pins); i++) 
    {
      readback = readback | ((digitalRead(in_pins[i]) << i));
    }
    if (debug) {
        Serial.println("Read data: " + String(readback, BIN));
    }
    digitalWrite(pinOE, HIGH);
    return readback;
}
//--------------------------------------------------------------------


//------------------------------------------------------------
// Test functions
//------------------------------------------------------------

//----- Data Bus Test function -------------------------------
// The purpose of this function is to test the Data bus 
// Any abdresse (passed as arg) can be used for that 
// so called "walking 1's" test
// return 0 if test ok
// return last wrote data  if nok

uint8_t DataBusTest(uint8_t Address)
{
  uint8_t readback;  
  setAddress(Address);
  Serial.println("\nDataBusTest Start - Address is : " + String(Address, BIN));
  
  for (uint8_t Data=1; Data < 0x10; Data <<= 1)
  {     
        if (debug)  Serial.println("DataBusTest - Data is : " + String(Data, BIN));
        writeData(Data);  
        if (readData() != Data) 
        {
          if (debug)  Serial.println("DataBusTest failed !");
          return (Data);
        }             
  }
  Serial.println("DataBusTest finished - ok");
  return (0);
}
//-------------------------------------------------------------


//----- Adress Bus Test function -----------------------------
//
//
uint8_t AdressBusTest()
{
  uint8_t Offset;
  uint8_t TestOffset;
  uint8_t Pattern     =  0xA;
  uint8_t AntiPattern =  0x5;
  uint8_t DataRead; 
  Serial.println("\nAdressBusTest Start ");
  
  //  Write the default pattern at each of the power-of-two offsets address
  for (Offset = 1; Offset != 0; Offset <<= 1)
  {
      if (debug)  Serial.println("Address is : " + String(Offset, BIN) + " - Pattern is : " + String(Pattern, BIN));
      
      setAddress(Offset);
      writeData(Pattern);
      if (debug)
      {
        if (readData() != Pattern) 
        {
          Serial.println("AddressBusTest failed !");
          return (Pattern);
        } 
      }      
  } 

  // Check for address bits stuck high
  //------------------------------------
     
  // write AntiPattern @0x00   
  if (debug)  Serial.println("Address is : " + String(0, BIN) + " - AntiPattern is : " + String(AntiPattern, BIN));
  setAddress(0);
  writeData(AntiPattern);     

  // read each of the power-of-two offsets address to check that Pattern is still there
  for (Offset = 1; Offset !=0 ; Offset <<= 1)
  {
    setAddress(Offset);
    DataRead = readData();
    if (debug)  Serial.println("Address is : " + String(Offset, BIN) + " data read is : " + String(DataRead, BIN));
    if (DataRead != Pattern) 
    {
       Serial.println("AddressBusTest failed !\n");
       Serial.println("Address is : " + String(Offset, BIN) + " expected data is : " + String(Pattern, BIN) + " data read is : " + String(DataRead, BIN));
       return (Pattern);
    }
  }

  // Check for address bits stuck low or shorted
  //------------------------------------

  // write Pattern @0x00  
  if (debug)  Serial.println("Address is : " + String(0, BIN) + " - Pattern is : " + String(Pattern, BIN));
  setAddress(0);
  writeData(Pattern); 

  // At each of the power-of-two TestOffsets address   
  for (TestOffset = 1; TestOffset !=0 ; TestOffset <<= 1)
  {
    // write AntiPattern
    setAddress(TestOffset);
    writeData(AntiPattern);
    if (debug)  Serial.println("Address is : " + String(Offset, BIN) + " data written is : " + String(AntiPattern, BIN));
    
     // At each of the other power-of-two address  
    for (Offset = 1; Offset !=0 ; Offset <<= 1)
    {
      //  check if Pattern is still there
      setAddress(Offset);
      DataRead = readData();
      if (debug)  Serial.println("Address is : " + String(Offset, BIN) + " data read is : " + String(DataRead, BIN));
      if ((DataRead != Pattern) && (Offset != TestOffset))
      {
         Serial.println("AddressBusTest failed !\n");
         Serial.println("Address is : " + String(Offset, BIN) + " expected data is : " + String(Pattern, BIN) + " data read is : " + String(DataRead, BIN));
         return (Pattern);
      }
    }
    // write Pattern
    setAddress(TestOffset);
    writeData(Pattern);  
  }

  Serial.println("AddressBusTest finished - ok");
  return (0);
}


// ----- Device Test function -----------------------------
// Now that Data and Address bus are ok we must test that 
// every bit in the device is capable of holding both 0 and 1
uint8_t  DeviceTest()
{
  uint8_t Offset;
  uint8_t TestOffset;
  uint8_t Pattern;
  uint8_t AntiPattern;
  uint8_t DataRead; 

  Serial.println("\nDeviceTest Start ");
  // Fill memory with a known pattern.
  Serial.println(" - DeviceTest 1st pass");
  Pattern = 1;
  Offset = 0;
  while (true) 
  {  
    setAddress(Offset);
    writeData(Pattern);
    Offset++; 
    Pattern++;
    Pattern = Pattern & 0x0F; // mask the 4 HSB to 0 as variable is byte and data is 4 bits
    if (Offset == 0xFF) break;
  }
  
  // Check each location and invert Pattern for the second pass.
  Serial.println(" - DeviceTest 2nd pass");
  Pattern = 1;
  Offset = 0;
  while (true) 
  {
    setAddress(Offset);
    DataRead = readData();     
    if (DataRead != Pattern)
    {
      Serial.println("DeviceTest failed !\n");
      Serial.println("Address is : " + String(Offset, BIN) + " expected data is : " + String(Pattern, BIN) + " data read is : " + String(DataRead, BIN));     
      return (Pattern);
    }
    AntiPattern = ~Pattern;
    AntiPattern = AntiPattern & 0x0F; // mask the 4 HSB to 0 as variable is byte and data is 4 bits
    setAddress(Offset);
    writeData(AntiPattern);
    Pattern++;
    Pattern = Pattern & 0x0F; // mask the 4 HSB to 0 as variable is byte and data is 4 bits
    Offset++;
    if (Offset == 0xFF) break;
  }

  //Check each location for the inverted pattern and zero it.
  Serial.println(" - DeviceTest 3rd pass");
  Pattern = 1;
  Offset = 0;  
  while (true)
  {
    AntiPattern = ~Pattern;
    AntiPattern = AntiPattern & 0x0F; // mask the 4 HSB to 0 as variable is byte and data is 4 bits
    setAddress(Offset);
    DataRead = readData();     
    if (DataRead != AntiPattern)
    {
      Serial.println("DeviceTest failed !\n");
      Serial.println("Address is : " + String(Offset, BIN) + " expected data is : " + String(AntiPattern, BIN) + " data read is : " + String(DataRead, BIN));     
      return (AntiPattern);
    } 
    Pattern++;
    Pattern = Pattern & 0x0F; // mask the 4 HSB to 0 as variable is byte and data is 4 bits
    Offset++;
    if (Offset == 0xFF) break;       
  }
  Serial.println("DeviceTest finished - ok\n");
  return (0);
}


// ----- Basic Test function (for a very quick test) -------
// write x @ x 
void singleTest()
{
  if (debug)  Serial.println("\nSingleTest write");
  for (uint8_t address = 0; address <= 0xF; address++)
  {
    setAddress(address);
    writeData(address);  
  }
  if (debug)  Serial.println("\nSingleTest read");
  for (uint8_t address = 0; address <= 0xF; address++)
  {
    setAddress(address);
    if ((readData() != address) && debug) Serial.println("Nok !");
  }
}


// ------------------------------------------------------------------------------
// setup Arduino pins direction and perform a basic "write 00 @ 00 and read" test
// ------------------------------------------------------------------------------
void setup() 
{
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("Initialising:"); //debug
    
    //set chip select and enable/write to output and configure
    pinMode(pinOE, OUTPUT);
    pinMode(pinWE, OUTPUT);
    pinMode(pinCS1, OUTPUT);
    pinMode(pinCS2, OUTPUT);
    digitalWrite(pinOE, HIGH); //output disable 
    digitalWrite(pinWE, HIGH); //read mode (active high)
    digitalWrite(pinCS1, LOW); //_CS1 enable (active low)
    digitalWrite(pinCS2, HIGH); //CS2 enable (active high)
    
    //set address pins as output and 0x00
    for (int i = 0; i < sizeof(addr_pins); i++)
    {
      pinMode(addr_pins[i], OUTPUT);
      digitalWrite(addr_pins[i], LOW);
    }

    //set DIx pins as output (from Arduino side) and low
    for (int i = 0; i < sizeof(out_pins); i++)
    {
      pinMode(out_pins[i], OUTPUT);
      digitalWrite(out_pins[i], LOW);
    }

    // Write in RAM (0x0 at @0x00) and prepare to read
    digitalWrite(pinOE, HIGH); //output disable (active high)
    digitalWrite(pinWE, LOW);  //write mode (active low)
    delay(1);                  //hold Data 1ms (tWD is 400ns in 5101 Datasheet)
    digitalWrite(pinWE, HIGH); //read mode (active high)
    digitalWrite(pinOE, LOW); //output enable (active low)
    delay(1);                  //hold Data 1ms (tOD is 350ns in 5101 Datasheet)  

    
    //set DOx pins as input (from Arduino side) and check DOx 0x0 value
    for (int i = 0; i < sizeof(in_pins); i++)
    {
      pinMode(in_pins[i], INPUT_PULLUP);
      uint8_t readback = digitalRead(in_pins[i]);
      if (readback == 0) {
        Serial.println("Initialisation check: pin " + String(i, DEC) + " LOW passed");
      } else {
        Serial.println("Initialisation check: pin " + String(i, DEC) + " LOW failed");
        failed++;
      }
    }
}

//----------------------------------------------------------------
//----------------------- Core loop ------------------------------
//----------------------------------------------------------------
void loop()
{
  if ((failed == 0) || (debug == true)) 
  {
    unsigned long duration = micros();
    
    // call the 3 test functions
    failed += DataBusTest(0x00);
    failed += AdressBusTest();
    failed += DeviceTest(); 
   
    duration = micros() - duration;
    Serial.println("Duration: " + String((double)duration/1000000, 2) + " s");
  }
  if (failed > 0) {
    Serial.println("\nGlobal Result: Failed (" + String(failed, DEC) + ")");
  } else {
    Serial.println("\nGlobal Test Result: Passed");
  }
  //digitalWrite(LED_BUILTIN, HIGH);
  while(1); //end
}
