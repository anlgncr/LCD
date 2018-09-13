#include "LCD.h"
// Resolution 84x48

LCD::LCD(uint16_t length){
	
	sprite_length = length;
	sprites = (Sprite**)calloc(sprite_length, sizeof(Sprite*));
	
	SPI.begin();
	LcdBaslat();
	LcdTemizle();
}

LCD::Sprite* LCD::newSprite(uint8_t* address)
{
	if(last_index >= sprite_length)
		return false;
	
	Sprite* my_sprite = (Sprite*)calloc(1, sizeof(Sprite));
	my_sprite->image = address;
	my_sprite->visible = true;
	
	sprites[last_index++] = my_sprite;
}

void LCD::newText(TextBox* myTextBox, uint8_t cols, uint8_t rows)
{	
	if(!rows || !cols)
		return;
	
	myTextBox->text = (uint8_t*)calloc(cols*rows+2, sizeof(uint8_t));
	myTextBox->text[0] = cols;
	myTextBox->text[1] = rows * 8;
	myTextBox->size = cols * rows;
	myTextBox->readFromSram = true;
	myTextBox->visible = true;
}

LCD::Sprite** LCD::getPool(){
	return sprites;
}

void LCD::textWrite(TextBox* myTextBox, char* text)
{
	if(myTextBox == NULL)
		return;
	
	uint8_t letter_count = (myTextBox->size) / 6; 
	uint8_t remaining = (myTextBox->size) % 6;
	uint8_t letter_index = 0;
	uint8_t* my_text = myTextBox->text + 2;
		
	for(letter_index = 0; letter_index < letter_count; letter_index++)
	{
		for(byte data_letter = 0; data_letter <5; data_letter++){
			*(my_text++) =  pgm_read_byte(&ASCII[text[letter_index] - 0x20][data_letter]);
		}
		*(my_text++) = 0x00;
	}

	for(uint8_t i = 0; i < remaining; i++)
		*(my_text++) = pgm_read_byte(&ASCII[text[letter_index] - 0x20][i]);
}

bool LCD::add(Sprite* my_sprite)
{
	if(last_index >= sprite_length)
		return false;
	
	for(uint16_t i=0; i<last_index; i++)//Check if your my_sprite is already in the pool, if so do not add it
		if(sprites[i] == my_sprite)
			return false;
	
	sprites[last_index++] = my_sprite;//Add my_sprite to the pool
	return true;
}

bool LCD::remove(Sprite* my_sprite)
{
	if(last_index <= 0)
		return false;
	
	for(uint16_t i=0; i<last_index; i++){
		if(sprites[i] == my_sprite){
			sprites[i] = NULL;
			dispose();
			return true;
		}
	}
	return false;
}

void LCD::dispose()
{
	uint16_t current_index = 0;
	
	for(uint16_t i=0; i<sprite_length; i++){
		if(sprites[i] != NULL){
			if(current_index != i){
				sprites[current_index] = sprites[i];
				sprites[i] = NULL;
			}
			current_index++;
		}
	}
	last_index = current_index;
}

bool LCD::update()
{
	if(last_index <= 0)
		return false;

	clearBuffer();
	for(uint16_t i=0; i<last_index; i++)
	{
		if(sprites[i] == NULL){
			dispose();
		}
		else if(sprites[i]->visible)
		{
			if(sprites[i]->readFromSram){
				drawObjectFromSram(sprites[i]->image, sprites[i]->x, sprites[i]->y, sprites[i]->inverse);
			}
			else{
				drawObject(sprites[i]->image, sprites[i]->x, sprites[i]->y, sprites[i]->inverse);
			}
		}
	}
	draw();
	
	return true;
}

void LCD::LcdKarakter(char karakter){
  LcdGonder(LCD_D, 0x00);
  
  for (int index = 0; index < 5; index++)
    //LcdGonder(LCD_D, ASCIII[karakter - 0x20][index]);
  
  LcdGonder(LCD_D, 0x00);
}

void LCD::LcdTemizle(){
  for (int index = 0; index < LCD_X * LCD_Y / 8; index++)
	LcdGonder(LCD_D, 0x00);
}

void LCD::LcdBaslat(){
  pinMode(PIN_SCE,   OUTPUT);
  //pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC,    OUTPUT);

  //digitalWrite(PIN_RESET, LOW);
  //digitalWrite(PIN_RESET, HIGH);

  LcdGonder( LCD_CMD, 0x21 );  // LCD Extended Commands.
  LcdGonder( LCD_CMD, 0xBf );  // Set LCD Vop (Contrast). //B1
  LcdGonder( LCD_CMD, 0x04 );  // Set Temp coefficent. //0x04
  LcdGonder( LCD_CMD, 0x14 );  // LCD bias mode 1:48. //0x13
  LcdGonder( LCD_CMD, 0x0C );  // LCD in normal mode. 0x0d for inverse
  LcdGonder( LCD_C, 0x20);
  LcdGonder( LCD_C, 0x0C);
}

void LCD::yaz(char *karakterler){
  while (*karakterler)
    LcdKarakter(*karakterler++);
}

void LCD::LcdGonder(byte dc, byte data){
  digitalWrite(PIN_DC, dc);
  PORTD &= ~(1 << PIN_SCE);
  SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(data);
  SPI.endTransaction();
  PORTD |= (1 << PIN_SCE);
}

/* x menzili: 0 dan 84 e kadar
   y menzili: 0 dan 5 e kadar */
void LCD::git(byte x, byte y){
  LcdGonder( 0, 0x80 | x);  // Sütün
  LcdGonder( 0, 0x40 | y);  // Satir  
}

void LCD::draw(){
	git(0,0);

	for(byte rows=0; rows<6; rows++)
		for(byte cols=0; cols<84; cols++)
			LcdGonder(1, buf[rows][cols]);
}

void LCD::clearBuffer(){
	for(byte rows=0; rows<6; rows++)
		for(byte cols=0; cols<84; cols++)
			buf[rows][cols]=0;
}


void LCD::drawObject(byte* image, int16_t x, int16_t y, bool inverse)
{	
	bool outOfScreen = false;
	
	int16_t sprite_width = pgm_read_byte(image);
	int16_t sprite_height = pgm_read_byte(image + 1) / 8;
	
	if(sprite_height < 1)
		sprite_height = 1;
	
	int16_t sprite_start_col = 0;
	int16_t sprite_end_col = sprite_width;
	int16_t buffer_start_col = 0;
	
	if(x<0 && ((x+sprite_width) > COLS)){//Serial.println("Sprite is overflowed from both left and right borders");
		sprite_start_col = abs(x);	//ilerle +x
		sprite_end_col = COLS - x; //ilerle COLS+x
	}
	else if(x < 0){ // Sprite ekranın sol tarafından taşmışsa 
		if((x + sprite_width) <= 0){//Serial.println("Sprite is out of left border");
			outOfScreen = true;
		}
		else{//Serial.println("Sprite is overflowed from left border");
			sprite_start_col = abs(x);
		}
	}
	else if(x + sprite_width > COLS){	// Sprite ekranın sag tarafından taşmışsa 
		if(x >= COLS){//Serial.println("Sprite is out of right border");
			outOfScreen = true;
		}
		else{//Serial.println("Sprite is overflowed from right border");
			sprite_end_col = COLS - x;
			buffer_start_col = x;
		}		
	}
	else{
		buffer_start_col = x;
	}

	
	if(!outOfScreen)
	{
		int16_t buffer_start_row;
		int8_t sprite_shift;
		int16_t sprite_start_row;
		int16_t buffer_end_row;
		
		if(y < 0){ //Serial.println("Ussten tasti");
			buffer_start_row = y/8 - 1;
			sprite_shift = 8 - abs(y)%8;
			sprite_start_row = abs(y)/8;
			buffer_end_row = buffer_start_row + sprite_height;
			buffer_start_row = -1;
		}
		else{
			buffer_start_row = y/8;
			sprite_shift = abs(y)%8;
			sprite_start_row = 0;
			buffer_end_row = buffer_start_row + sprite_height;
		}
		
		if(buffer_end_row < 0 || buffer_start_row > LAST_ROW){//Serial.println("Disarida");
			outOfScreen = true;
		}
		
		if(!outOfScreen)
		{
			uint8_t (*sprite)[sprite_width];
			sprite = (uint8_t(*)[sprite_width])(image+2);
			
			if(buffer_end_row > LAST_ROW){//Serial.println("Alttan tasti");
				buffer_end_row = LAST_ROW + 1;
			}
			
			int16_t next_sprite_row = sprite_start_row;	
			for(int rows = buffer_start_row; rows < buffer_end_row; rows++)
			{
				if(rows >= 0)
				{
					int16_t next_buffer_col = 0;
					for(int cols = sprite_start_col; cols< sprite_end_col ; cols++)
					{
						uint8_t data = pgm_read_byte(&sprite[next_sprite_row][cols]) << sprite_shift;
						buf[rows][buffer_start_col + next_buffer_col] |= (inverse)? ~data: data;
						next_buffer_col++;
					}
				}
				if((rows + 1) >= 0 && ((rows + 1) <= LAST_ROW) && sprite_shift)
				{
					int16_t next_buffer_col = 0;
					for(int cols = sprite_start_col; cols < sprite_end_col; cols++)
					{
						uint8_t data= pgm_read_byte(&sprite[next_sprite_row][cols]) >> (8 - sprite_shift);
						buf[rows + 1][buffer_start_col + next_buffer_col] |= (inverse)? ~data: data;
						next_buffer_col++;
					}
				}
				next_sprite_row++;
			}
		}
	}
}


void LCD::drawObjectFromSram(byte* image, int16_t x, int16_t y, bool inverse)
{	
	bool outOfScreen = false;
	
	int16_t sprite_width = image[0];
	int16_t sprite_height = image[1] / 8;
	
	if(sprite_height < 1)
		sprite_height = 1;
	
	int16_t sprite_start_col = 0;
	int16_t sprite_end_col = sprite_width;
	int16_t buffer_start_col = 0;
	
	if(x<0 && ((x+sprite_width) > COLS)){//Serial.println("Sprite is overflowed from both left and right borders");
		sprite_start_col = abs(x);	//ilerle +x
		sprite_end_col = COLS - x; //ilerle COLS+x
	}
	else if(x < 0){ // Sprite ekranın sol tarafından taşmışsa 
		if((x + sprite_width) <= 0){//Serial.println("Sprite is out of left border");
			outOfScreen = true;
		}
		else{//Serial.println("Sprite is overflowed from left border");
			sprite_start_col = abs(x);
		}
	}
	else if(x + sprite_width > COLS){	// Sprite ekranın sag tarafından taşmışsa 
		if(x >= COLS){//Serial.println("Sprite is out of right border");
			outOfScreen = true;
		}
		else{//Serial.println("Sprite is overflowed from right border");
			sprite_end_col = COLS - x;
			buffer_start_col = x;
		}		
	}
	else{
		buffer_start_col = x;
	}

	
	if(!outOfScreen)
	{
		int16_t buffer_start_row;
		int8_t sprite_shift;
		int16_t sprite_start_row;
		int16_t buffer_end_row;
		
		if(y < 0){ //Serial.println("Ussten tasti");
			buffer_start_row = y/8 - 1;
			sprite_shift = 8 - abs(y)%8;
			sprite_start_row = abs(y)/8;
			buffer_end_row = buffer_start_row + sprite_height;
			buffer_start_row = -1;
		}
		else{
			buffer_start_row = y/8;
			sprite_shift = abs(y)%8;
			sprite_start_row = 0;
			buffer_end_row = buffer_start_row + sprite_height;
		}
		
		if(buffer_end_row < 0 || buffer_start_row > LAST_ROW){//Serial.println("Disarida");
			outOfScreen = true;
		}
		
		if(!outOfScreen)
		{
			uint8_t (*sprite)[sprite_width];
			sprite = (uint8_t(*)[sprite_width])(image+2);
			
			if(buffer_end_row > LAST_ROW){//Serial.println("Alttan tasti");
				buffer_end_row = LAST_ROW + 1;
			}
			
			int16_t next_sprite_row = sprite_start_row;	
			for(int rows = buffer_start_row; rows < buffer_end_row; rows++)
			{
				if(rows >= 0)
				{
					int16_t next_buffer_col = 0;
					for(int cols = sprite_start_col; cols< sprite_end_col ; cols++)
					{
						uint8_t data = sprite[next_sprite_row][cols] << sprite_shift;
						buf[rows][buffer_start_col + next_buffer_col] |= (inverse)? ~data: data;
						next_buffer_col++;
					}
				}
				if((rows + 1) >= 0 && ((rows + 1) <= LAST_ROW) && sprite_shift)
				{
					int16_t next_buffer_col = 0;
					for(int cols = sprite_start_col; cols < sprite_end_col; cols++)
					{
						uint8_t data= sprite[next_sprite_row][cols] >> (8 - sprite_shift);
						buf[rows + 1][buffer_start_col + next_buffer_col] |= (inverse)? ~data: data;
						next_buffer_col++;
					}
				}
				next_sprite_row++;
			}
		}
	}
}

int LCD::getCenterX(int my_cols){
	return (COLS / 2 - my_cols / 2);  
}

int LCD::getCenterY(int my_rows){
	return (ROWS / 2 - my_rows / 2);  
}

int LCD::getRightX(int my_cols){
	return (COLS - my_cols);  
}

int LCD::getBottomY(int my_rows){
	return (ROWS - my_rows);
}

uint16_t LCD::getIndex(){
	return last_index;
}			





















