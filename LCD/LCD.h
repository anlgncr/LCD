#ifndef LCD_h
#define LCD_h
#include "arduino.h"
#include <SPI.h>
#include "ASCII.h"
#define PIN_SCE   2  // LCD CS  .... Pin 3
#define PIN_RESET 5  // LCD RST .... Pin 1
#define PIN_DC    4  // LCD Dat/Com. Pin 5
						// LCD SPIDat . Pin 6
						// LCD SPIClk . Pin 4
                     // LCD Gnd .... Pin 2
                     // LCD Vcc .... Pin 8
                     // LCD Vlcd ... Pin 7
#define LCD_C     LOW
#define LCD_D     HIGH
#define LCD_X     84

#define LCD_Y     48
#define LCD_CMD   0
#define COLS	84
#define ROWS	48
#define LAST_ROW 5

class LCD
{
  public:
	LCD(uint16_t);
	
	struct Sprite{
		uint8_t* image;
		int x;
		int y;
		bool visible;
		bool inverse;
		bool readFromSram;
	};
	
	struct TextBox{
		uint8_t* text;
		int x;
		int y;
		bool visible;
		bool inverse;
		bool readFromSram;
		uint8_t size;
	};
	
	Sprite* newSprite(uint8_t*);
	int getCenterX(int);
	int getCenterY(int);
	int getRightX(int);
	int getBottomY(int);
	
	Sprite** getPool();
	
	void newText(TextBox*, uint8_t, uint8_t);
	void textWrite(TextBox*, char*);
	
	uint16_t getIndex();
	bool add(Sprite*);
	bool remove(Sprite*);
	bool update();
	
	void LcdTemizle();
	void yaz(char*);
	void LcdGonder(byte,byte);
	void git(byte,byte);
	void draw();
	
	
  private:
	Sprite** sprites; 
	uint16_t sprite_length = 0;
	uint16_t last_index = 0;
	
	void drawObject(byte*, int16_t, int16_t, bool);
	void drawObjectFromSram(byte*, int16_t, int16_t, bool);
	void dispose();
  
	void LcdBaslat();
	void LcdKarakter(char);
	void clearBuffer();
	bool informMe = true;
	
	byte buf[6][84];    
};

#endif