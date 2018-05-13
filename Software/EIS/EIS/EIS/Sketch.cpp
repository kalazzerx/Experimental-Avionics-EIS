#include <Arduino.h>
#include <LiquidCrystal.h>
#include "Ads1118.h"
#include <SPI.h>

#define CS 9
#define MAX_PAGES 16
const int rs = 42, rw = 43, en = 45, d0 = 46, d1 = 47, d2 = 48, d3 = 49, d4 = 13, d5 = 12, d6 = 11, d7 = 10;
LiquidCrystal lcd(rs, rw, en, d0, d1, d2, d3, d4, d5, d6, d7);
Ads1118 ads1118(CS); // instantiate an instance of class Ads1118

int loop_count = 1;

enum MODE_T
{
	DISPLAY_MODE,
	CONFIG_MODE,
	SET_LIMITS_MODE,
	SAVE_LEAN_POINT_MODE
};

bool button_previous[3] = {1, 1, 1};
bool button_press_event[3] = {0, 0, 0};
bool button_release_event[3] = {0, 0, 0};
int button_pin[3] = {28, 27, 26};
bool button[3] = {1, 1, 1};
MODE_T mode = DISPLAY_MODE;
	
int rpm = 0;
int oilT = 59;
int oilP = 99;
float cht[6] = {0, 0, 0, 0, 0, 0};
float egt[6] = {0, 0, 0, 0, 0, 0};
float internal_temp = 0;
float hours = 0;

int currentPage = 1;
int previousPage = 99;

float voltToDegree(float volt)
{
	float temp_c = (volt-1.25)/0.005;
	return (temp_c * 1.8) + 32;
}

void readButtons()
{
  for (int i = 0; i < 3; i++)
  {
    button[i] = digitalRead(button_pin[i]);
    if (button_previous[i] != button[i])
    {
      if (button[i] == 0)
      {
        button_press_event[i] = 1;
		delay(20); // debounce
      }
      if (button[i] == 1)
      {
        button_release_event[i] = 1;
		delay(20); // debounce
      }
      button_previous[i] = button[i];
    }
  }
}

void readSensors(int i)
{
	if (i == 1)
	{
		digitalWrite(33, LOW);
		digitalWrite(34, LOW);
		digitalWrite(35, LOW);
		cht[0] = voltToDegree(ads1118.adsRead(ads1118.AIN0));
		egt[0] = voltToDegree(ads1118.adsRead(ads1118.AIN1));

		digitalWrite(33, HIGH);
		digitalWrite(34, LOW);
		digitalWrite(35, LOW);
		cht[1] = voltToDegree(ads1118.adsRead(ads1118.AIN0));
		egt[1] = voltToDegree(ads1118.adsRead(ads1118.AIN1));
	}
	if (i == 2)
	{
		digitalWrite(33, LOW);
		digitalWrite(34, HIGH);
		digitalWrite(35, LOW);
		cht[2] = voltToDegree(ads1118.adsRead(ads1118.AIN0));
		egt[2] = voltToDegree(ads1118.adsRead(ads1118.AIN1));
	
		digitalWrite(33, HIGH);
		digitalWrite(34, HIGH);
		digitalWrite(35, LOW);
		cht[3] = voltToDegree(ads1118.adsRead(ads1118.AIN0));
		egt[3] = voltToDegree(ads1118.adsRead(ads1118.AIN1));
	}
	if (i == 3)
	{
		digitalWrite(33, LOW);
		digitalWrite(34, LOW);
		digitalWrite(35, HIGH);
		cht[4] = voltToDegree(ads1118.adsRead(ads1118.AIN0));
		egt[4] = voltToDegree(ads1118.adsRead(ads1118.AIN1));

		digitalWrite(33, HIGH);
		digitalWrite(34, LOW);
		digitalWrite(35, HIGH);
		cht[5] = voltToDegree(ads1118.adsRead(ads1118.AIN0));
		egt[5] = voltToDegree(ads1118.adsRead(ads1118.AIN1));
	}
	
	digitalWrite(33, LOW);
	digitalWrite(34, LOW);
	digitalWrite(35, LOW);
}

void clearScreen(int index)
{
  if (previousPage != currentPage)
  {
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
  previousPage = index;
  
  lcd.setCursor(0,0);
}

// Endur, Fuel, and Flow
void displayPage1()
{
  clearScreen(1);
  
  lcd.print(" 0:00  15.3  0.0");
  lcd.setCursor(0, 1);
  lcd.print("Endur  Fuel Flow");
}

// User definable 1
void displayPage2()
{

  clearScreen(2);
  lcd.print("   0  0.0  0.0  ");
  lcd.setCursor(0,1);
  lcd.print(" 59/99  52    53");

}

// User definable 2
void displayPage3()
{
	clearScreen(3);
	lcd.print("   0    0   1050");
	lcd.setCursor(0,1);
	lcd.print(" 59/99  53    54");
}

// EGT/CHT bar graph
void displayPage4()
{
	clearScreen(4);
	//byte graph1[8] = {B10000, B10000, B10000, B10000, B10000, B10000, B00000, B00000};
	//lcd.createChar(0, graph1);
	
	cht[0] = 20;
	cht[1] = 60;
	
	uint64_t cht_graphs[6] = {0, 0, 0, 0, 0, 0};
	
	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < cht[i]/20; j++)
		{
			cht_graphs[i] |= (0x1 << j);
		}
	}
	
	for (int i = 0; i < 8; i++)
	{
		byte graph[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		for (int j = 0; j < 6; j++)
		{
		    graph[j] = (cht_graphs[j] >> ( 5 * i)) & 0b11111;
		}
		lcd.createChar(i, graph);
		lcd.setCursor(i, 0);
		lcd.write(byte(0));
	}
	
	//lcd.print((int)cht_graphs[1], HEX);
	// this line is required to set the LCD back into DDRAM mode
	// https://forum.arduino.cc/index.php?topic=262753.0
	
	lcd.setCursor(8, 0);
	lcd.print("  0    0");
	lcd.setCursor(0,1);
	lcd.print(" 0.0    54     0");
}

// EGT cruise bar graph
void displayPage5()
{
	clearScreen(5);

	lcd.print("          0    0");
	lcd.setCursor(0,1);
	lcd.print(" 0.0    54     0");
}

int calcOffset(float i, int decimals, int width)
{
	if (decimals > 0)
	{
		width-= decimals + 1;
	}
	width--; // for one's position
	if ( i >= 10)
	{
		width--;
	}
	if (i >= 100)
	{
		width--;
	}
	if (i >= 1000)
	{
		width--;
	}
	return width;
}
// EGT page
void displayPage6()
{
	clearScreen(6);
	
	lcd.setCursor(0 + calcOffset(egt[0], 0, 4),0);
	lcd.print(egt[0], 0);

	lcd.setCursor(5 + calcOffset(egt[1], 0, 4), 0);
	lcd.print(egt[1], 0);

	lcd.setCursor(10 + calcOffset(egt[2], 0, 4), 0);
	lcd.print(egt[2], 0);

	lcd.setCursor(0 + calcOffset(egt[3], 0, 4), 1);
	lcd.print(egt[3], 0);

	lcd.setCursor(5 + calcOffset(egt[4], 0, 4), 1);
	lcd.print(egt[4], 0);

	lcd.setCursor(10 + calcOffset(egt[5], 0, 4), 1);
	lcd.print(egt[5], 0);
	
	lcd.setCursor(15, 0);
	lcd.print("E");
	lcd.setCursor(15, 1);
	lcd.print("G");
}

// Digital leaning page
void displayPage7()
{
	clearScreen(7);
	lcd.print("  54   53   54 0");
	lcd.setCursor(0, 1);
	lcd.print("  53   53   53 L");
}

// Cruise monitor page
void displayPage8()
{
	clearScreen(8);
	lcd.print("---- ---- ---- C");
	lcd.setCursor(0,1);
	lcd.print("---- ---- ---- Z");
}

// CHT page
void displayPage9()
{
	clearScreen(9);
	
	float highest = cht[0];
	for (int i = 0; i < 6; i++)
	{
		if (cht[i] > highest)
		{
			highest = cht[i];
		}
	}
	
	lcd.setCursor(0 + calcOffset(cht[0], 0, 3), 0);
	lcd.print(cht[0], 0);

	lcd.setCursor(4 + calcOffset(cht[0], 0, 3), 0);
	lcd.print(cht[1], 0);

	lcd.setCursor(8 + calcOffset(cht[0], 0, 3), 0);
	lcd.print(cht[2], 0);
	
	lcd.setCursor(13 + calcOffset(highest, 0, 3), 0);
	lcd.print(highest, 0);

	lcd.setCursor(0 + calcOffset(cht[0], 0, 3), 1);
	lcd.print(cht[3], 0);

	lcd.setCursor(4 + calcOffset(cht[0], 0, 3), 1);
	lcd.print(cht[4], 0);

	lcd.setCursor(8 + calcOffset(cht[0], 0, 3), 1);
	lcd.print(cht[5], 0);
	
	lcd.setCursor(13, 1);
	
	lcd.print("CHT");
}

// Alt, VSI, Asp, and H20
void displayPage10()
{
	clearScreen(10);
	lcd.print(" 1050  0 ---  59");
	lcd.setCursor(0,1);
	lcd.print("Alt  VSI MPH H20");
}

void displayPage11()
{
	clearScreen(11);
	lcd.print(" 0:29:00     71");
	lcd.setCursor(15, 0);
	lcd.write((byte)0xDF); // degree character
	
	lcd.setCursor(0 + calcOffset(hours, 1, 6), 1);
	lcd.print(hours, 1);
	
	lcd.setCursor(7, 1);
	lcd.print("Hrs  Unit");
}

void displayPage12()
{
	clearScreen(12);
	
	int rpmOffset = 0;
	if (rpm >= 1000)
	{
		// no changes needed
	}
	else if (rpm >= 100)
	{
		rpmOffset += 1;
	}
	else if (rpm >= 10)
	{
		rpmOffset += 2;
	}
	else
	{
		rpmOffset += 3;
	}
	for (int i = 0; i < rpmOffset; i++)
	{
		lcd.print(" ");
	}
	lcd.print(rpm);

	lcd.setCursor(7, 0);
	int oilTOffset = 0;
	if (oilT >= 100)
	{
		// no changes needed
	}
	else if (oilT >= 10)
	{
		oilTOffset += 1;
	}
	else
	{
		oilTOffset += 2;
	}
	for (int i = 0; i < oilTOffset; i++)
	{
		lcd.print(" ");
	}
	//lcd.setCursor(oilTOffset, 0);
	lcd.print(oilT);

	lcd.setCursor(14, 0);
	int oilPOffset = 0;
	if (oilP >= 10)
	{
		// no change needed
	}
	else
	{
		oilPOffset += 1;
	}
	for (int i = 0; i < oilPOffset; i++)
	{
		lcd.print(" ");
	}
	//lcd.setCursor(oilTOffset, 0);
	lcd.print(oilP);
	  
	lcd.setCursor(0, 1);
	lcd.print("RPM   OilT  OilP");
}

void displayPage13()
{
	clearScreen(13);
	lcd.print("0.0   0.0   0.0");
	lcd.setCursor(0,1);
	lcd.print("AX1   AX2   AX3");
}

// FPR, Fuel, and Aux3
void displayPage14()
{
	clearScreen(14);
	lcd.print("0.0   0.0   0.0");
	lcd.setCursor(0,1);
	lcd.print("AX4   AX5   AX6");
}

// OAT F, OAT C, and Volt
void displayPage15()
{
	clearScreen(15);
	char text[17] = "-50 F -50 C 12.4";
	text[3] = 0b11011111; // degree symbol
	text[9] = 0b11011111; // degree symbol
	lcd.print(text);
	lcd.setCursor(0,1);
	lcd.print(" OAT   OAT  Volt");
}

// CRate, EgtSp, and Carb
void displayPage16()
{
	clearScreen(16);
	
	lcd.print("  0       1  -50");
	lcd.setCursor(0, 1);
	lcd.print("CRate EgtSp Carb");
}

#define MAX_CONFIG_PAGES 43
char* configItems[MAX_CONFIG_PAGES] = {"Cont", "Back_Light", "Alt", "Fuel", "Max Time", \
	                                   "Interval", "Max Flow", "Max OilP", "Min OilP", "Min Crz_OP", \
									   "Max OilT", "Min OilT", "Max RPM", "Min RPM", "Min Fuel", \
									   "Min Aux 1", "Min Aux 2", "Min Aux 3", "Min Aux 4", "Min Aux 5", \
									   "Min Aux 6", "Max Aux 1", "Max Aux 2", "Max Aux 3", "Max Aux 4",  \
									   "Max Aux 5", "Max Aux 6", "Max H20", "Min H20", "Max Volt", \
									   "Min Volt", "Max Carb", "Min Carb", "Max EGT", "Min EGT", \
									   "Lim-RPM", "Max EgtSpan", "Max EGT-Inc", "Max EGT-Dec", "Max Crate", \
									   "Max CHT", "Min CHT", "Display"};
int currentConfigPage = 1;
//Config
void displayPage17()
{
  clearScreen(17);
  lcd.print(configItems[currentConfigPage-1]);
  lcd.setCursor(15, 0);
  lcd.print("3");
  lcd.setCursor(0, 1);
  lcd.print("  Up  Down  Next");
}

void setup() {
  // put your setup code here, to run once:
  pinMode(44, OUTPUT); // LCD brightness control
  analogWrite(44, 255);

  pinMode(26, INPUT_PULLUP);
  pinMode(27, INPUT_PULLUP);
  pinMode(28, INPUT_PULLUP);
  
  // Warning Light
  pinMode(5, OUTPUT);

  ads1118.begin();
  pinMode(33, OUTPUT);
  pinMode(34, OUTPUT);
  pinMode(35, OUTPUT);
  digitalWrite(33, LOW);
  digitalWrite(34, LOW);
  digitalWrite(35, LOW);
  ads1118.update_config(0x428B);

  Serial2.begin(9600);
  lcd.begin(16, 2);
  
  for (int i = 0; i < 16; i ++)
  {
     lcd.setCursor(i, 0);
     lcd.write((0b1101 << 8) + i);
	 lcd.setCursor(i, 1);
	 lcd.write((0b1100 << 8) + i);
  }
  
  while(true){}
	
	
}

void (*pageDisplayFunctions[MAX_PAGES])(void) = { displayPage1,  displayPage2,  displayPage3,  displayPage4,  displayPage5, \
	                                            displayPage6,  displayPage7,  displayPage8,  displayPage9,  displayPage10, \
	                                            displayPage11, displayPage12, displayPage13, displayPage14, displayPage15, \
                                                displayPage16 };

void processDisplayMode()
{

	// Buttons 1 and 2 pressed simultaneously
	// to request configuration menu
	if ((button_press_event[0] &&
	    !digitalRead(button_pin[1])) ||
	    (button_press_event[1] &&
	    !digitalRead(button_pin[0])))
	{
		button_press_event[0] = 0;
		button_press_event[1] = 0;
		mode = SET_LIMITS_MODE;
		return;
	}
	
	// Buttons 2 and 3 pressed simultaneously
	// to request configuration menu
	if ((button_press_event[1] &&
	!digitalRead(button_pin[2])) ||
	(button_press_event[2] &&
	!digitalRead(button_pin[1])))
	{
		button_press_event[1] = 0;
		button_press_event[2] = 0;
		mode = SAVE_LEAN_POINT_MODE;
		lcd.setCursor(0, 0);
		lcd.print("                ");
		lcd.setCursor(0, 1);
		lcd.print("                ");
		lcd.setCursor(0, 0);
		lcd.print("Save Lean Point?");
		lcd.setCursor(0, 1);
		lcd.print("  Yes Reset No  ");
		return;
	}
		  
	// Next/Ack button
	else if (button_press_event[0])
	{
		currentPage++;
		if (currentPage > MAX_PAGES)
		{
			previousPage = currentPage;
			currentPage = 1;
		}
		button_press_event[0] = 0;
	}
		  
	// Previous button
	else if (button_press_event[1])
	{
		currentPage--;
		if (currentPage < 1)
		{
			previousPage = currentPage;
			currentPage = MAX_PAGES;
		}
		button_press_event[1] = 0;
	}
	
	else if (!digitalRead(button_pin[2]))
	{
		if (currentPage == 2)
		{
			lcd.setCursor(0, 0);
			lcd.print("Tach Flow FUE   ");
			lcd.setCursor(0, 1);
			lcd.print("OT /OP CHH  EGH ");
			return;
		}
		else if (currentPage == 3)
		{
			lcd.setCursor(0, 0);
			lcd.print("Tach  VS   Alt  ");
			lcd.setCursor(0, 1);
			lcd.print("OT /OP CHH  EGH ");
			return;
		}
		else if (currentPage == 4 || currentPage ==5)
		{
			lcd.setCursor(0, 0);
			lcd.print("         1P EGPk");
			lcd.setCursor(0, 1);
			lcd.print("Flow  EGH   Tach");
			return;
		}
		
	}
	
	pageDisplayFunctions[currentPage-1]();
}

void processConfigMode()
{
	
	if (button_press_event[2])
	{
		button_press_event[2] = 0;
		currentConfigPage++;
		if (currentConfigPage > MAX_CONFIG_PAGES)
		{
			currentConfigPage = 1;
			mode = DISPLAY_MODE;
			return;
		}
	}
	
	displayPage17();
}

void processSaveLeanPointMode()
{
	if (button_press_event[0] ||
	    button_press_event[1] ||
		button_press_event[2])
	{
		button_press_event[0] = 0;
		button_press_event[1] = 0;
		button_press_event[2] = 0;
		mode = DISPLAY_MODE;
		return;
	}
}

void loop() {
	
  readButtons();
  // need to read only 1/3 of sensors each
  // loop to keep buttons responsive
  readSensors(loop_count);
  
  if (DISPLAY_MODE == mode)
  {
	  processDisplayMode();
  }
  else if (SET_LIMITS_MODE == mode)
  {
	  processConfigMode();
  }
  else if (SAVE_LEAN_POINT_MODE == mode)
  {
	  processSaveLeanPointMode();
  }
  
  
  loop_count++;
  if (loop_count > 3)
  {
	  loop_count = 1;
  }
}
