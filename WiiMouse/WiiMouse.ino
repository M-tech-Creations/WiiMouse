/*
 *Written by Mario Avenoso of Mario.MtechCreations.com
 *Project to use the Wii Nunchuck as a computer mouse using 
 *adafruit's Trinket
 *
 *This work is based on the work found HERE: http://www.windmeadow.com/node/42 Thanks!
 *Also from here http://todbot.com/blog/2008/02/18/wiichuck-wii-nunchuck-adapter-available/ Thanks!
 *and This instructable by Brian Krukoski http://www.instructables.com/id/USB-Wiichuck-Mouse-Using-an-Arduino-Leonardo/ Thanks!
 *2/9/14
 */
#include <TinyWireM.h> //TinyWireM Lib for the ATTiny - https://github.com/adafruit/TinyWireM
#include <TrinketMouse.h>//USB Mouse Lib for the Trinket - https://github.com/adafruit/Adafruit-Trinket-USB

static uint8_t Wii_buf[6];   // array to store nunchuck data, 
//[0] joy x; [1] joy y; [2] accel x;[3] accel y;[4] accel z;[5] buttons

//For Update cycle
long pretime = 0;//time at which the last event occurred 
long length = 5;//Length between event in MS

// parameters for reading the joystick:
int range = 20;                // output range of X or Y movement
int center = range/2;          // resting position value
int threshold = range/10;      // resting threshold

//int range_scroll = 10;               // range for scroll movement
//int center_scroll = range_scroll/2;  // set center 


//Scroll/Mouse switch
#define LED 1 //Trinket LED pin
int LED_state = HIGH;
bool tilt;//used for switching between scroll and mouse
bool is_switched = false;//used tio keep track of contoller position
int accelx_range = 160; //The tilt value to overcome to switch between scroll and mouse

void setup()
{
	pinMode(LED,OUTPUT);//used to show whether in scroll or mouse mode
	TrinketMouse.begin();
	TinyWireM.begin();
	Wii_start();
}

void loop()
{
	unsigned long currenttime = millis();//get the current time
	if(currenttime - pretime > length) 
	{
		// save the last time you blinked the LED
		pretime = currenttime;//sets the last event to current time
		if(WII_get_data())//sets Wii_buf data
		{
			mouse_move();
		}
		
	}
	//outside of loop so it will keep updating and will not let the computer think the device has an error
	if (Wii_CB() || Wii_ZB())
	{
		TrinketMouse.move(0, 0, 0, 1 + Wii_ZB()& 0x07);//left click
	}
	
	else
	{
		TrinketMouse.move(0, 0, 0, 0 & 0x07);//+ is right; + is down; + is scroll up; 1 is left click; 2 is right click
	}
	
	
	
}

static void Wii_start()
{
	TinyWireM.beginTransmission (0x52);	// transmit to device 0x52
	TinyWireM.write((uint8_t)0x40);// sends memory address
	TinyWireM.write((uint8_t)0x00);// sends sent a zero.
	TinyWireM.endTransmission ();	// stop transmitting
}

//would be send_zero()
static void WII_data_request() //sends zero byte to controller so it will send data back
{
	TinyWireM.beginTransmission(0x52);
	TinyWireM.write((uint8_t)0x00);// sends one byte
	TinyWireM.endTransmission();// stop transmitting
}

static int WII_get_data()
{
	int count = 0;
	
	TinyWireM.requestFrom (0x52, 6);// request data from Controller
	
	while(TinyWireM.available())
	{
		Wii_buf[count] = Wii_decode_byte(TinyWireM.read());
		count++;
	}
	WII_data_request();//send zero 
	if (count>=5)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// Encode data to format that most wiimote drivers except
// only needed if you use one of the regular wiimote drivers
char Wii_decode_byte (char x)
{
	x = (x ^ 0x17) + 0x17;
	return x;
}


// returns Z button state: 1 = pressed, 0 = not pressed
static int Wii_ZB()
{
	return ((Wii_buf[5] >> 0) & 1) ? 0 : 1;  // voodoo
}

// returns C button state: 1 = pressed, 0 = not pressed
static int Wii_CB()
{
	return ((Wii_buf[5] >> 1) & 1) ? 0 : 1;  // voodoo
}

// returns value of x-axis joystick
static int Wii_JX()
{
	return Wii_buf[0];
}

// returns value of y-axis joystick
static int Wii_JY()
{
	return Wii_buf[1];
}

// returns value of the x-axis accelerator 
static int Wii_Accelx()
{
	return Wii_buf[2];
}

//checks and updates moves movements 
static void mouse_move()
{
	int yReading_Scroll = 0;// Used for scroll range
	
	int xReading = Wii_JX(); //calls on nunchuck_funcs library for x axis reading
	//Serial.println(xReading);
	xReading = map(xReading, 20, 218, 0, range); // 20 and 218 arbitrarily determined through experimentation, maps to 0 and preset range
	
	int xDistance = xReading - center;
	
	if (abs(xDistance) < threshold) //if absolute value of xDistance is less than predefined threshold....
	{
		xDistance = 0;
	}

	// read the y axis
	int yReading = Wii_JY(); //calls on nunchuck_funcs library for y axis reading
	int yDistance = 0;
	yReading = map(yReading, 30, 229, 0, range); // 30 and 229 arbitrarily determined through experimentation, maps to 0 and preset range
	
	yDistance = yReading - center;
	
	
	if (abs(yDistance) < threshold) //if absolute value of yDistance is less than predefined threshold....
	{
		yDistance = 0;
	}
	
	if (Wii_Accelx()>accelx_range && tilt == false)
	{
		tilt = true;
		
		if (LED_state == LOW)//Switch status light
		{
			LED_state = HIGH;
			is_switched = true;
		}
		else
		{
			LED_state = LOW;
			is_switched = false;
		}
		digitalWrite(LED,LED_state);//toggle LED
	}
	if (Wii_Accelx()<accelx_range)
	{
		tilt=false;
	}
	
	if ((xDistance != 0) || (yDistance != 0)) {
		
		if (is_switched==false)//use joystick as a mouse
		{
			if (Wii_CB()||Wii_ZB())
			{
				TrinketMouse.move(xDistance, -yDistance, 0, 1 + Wii_ZB() & 0x07);//left click / right click
			}
			
			else
			{
				TrinketMouse.move(xDistance, -yDistance, 0, 0 & 0x07); // -yDistance ensures inverted style joystick
			}
		}
		else
		{
			
			
			yDistance = yReading/2 - center/2;//limit range
			
			if (Wii_CB() || Wii_ZB())
			{
				TrinketMouse.move(0, 0, yDistance, 1 + Wii_ZB() & 0x07);//left click / right click
			}
			
			else
			{
				TrinketMouse.move(0, 0, yDistance, 0 & 0x07); // -yDistance_Scroll ensures inverted style joystick
			}
		}
		
		
	}
	
}