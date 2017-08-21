//*******************************************************************
//Author: Taekwang jang
//Description: PRCv9E Extended Layer (GPIO, Power Switch, SPI)
//*******************************************************************
#include "mbus.h"
#include "PRCv9E.h"

#define SPI_DELAY 5
#define TEST_ITR  10

// SPI setting
#define SPI_MOSI  1
#define SPI_MISO  0	// SPI MISO, Expended in binary
#define SPI_CLK   5

// SPI0: Timer setting
#define SPI_SS_TIMER    13

// SPI1: SRAM setting
#define SPI_0 	1	// SPI: MOSI, SQI: SIO0, Marked: MOSI
#define SPI_1 	0	// SPI: MISO, SQI: SIO1, Marked: MISO
#define SPI_2 	2	// SPI: N/A,  SQI: SIO2, Marked: MOSI1
#define SPI_3 	4	// SPI: HOLD, SQI: SOI3, Marked: MOSI2
#define SPI_SS_SRAM 	7
#define MUX_SEL	6	// 0: From M3, 1: From AFE


// SPI2: Flash setting
#define SPI_SS_FLASH	12
#define SPI_MISO_FLASH	10
#define SPI_MOSI_FLASH	11
#define SPI_CLK_FLASH	9

// SPI3: AFE setting
#define SPI_SS_AFE	15


#define SLEEP_TIME	20

// SRAM Commands
#define SRAM_READ	0x00000003
#define SRAM_WRITE	0x00000002
#define SRAM_EDIO	0x0000003B
#define SRAM_EQIO	0x00000038
#define SRAM_RSTIO	0x000000FF
#define SRAM_RDMR	0x00000005
#define SRAM_WRMR	0x00000001

// Flash Commands
#define FLASH_WRITE_EN		0x06	//Write enable
#define FLASH_WRITE_DI		0x04	//Write disable
#define FLASH_READ_STAT		0x05	//Read Status Register
#define FLASH_WRITE_STAT	0x01	//Write Status register
#define FLASH_READ_DATA_BYTE	0x03
#define FLASH_READ_DATA_BYTE_HS	0x0B
#define	FLASH_DUAL_FAST_READ	0x3B
#define FLASH_DUAL_FAST_PROGRAM	0xA2
#define	FLASH_SUBSECTOR_ERASE	0x20
#define FLASH_BULT_ERASE	0x20
#define	FLASH_DEEP_PD		0xB9
#define	FLASH_RELEASE_DEEP_PD	0xAB
#define FLASH_READ_IDEN		0x9F
#define FLASH_PAGE_PROGRAM	0x02

volatile uint32_t* GPIO    = (volatile uint32_t *) 0xA6000008;
volatile uint32_t* GPIODIR = (volatile uint32_t *) 0xA6001000;
volatile uint32_t  GPIO_WRITE;
volatile uint32_t enumerated;

//***************************************************
//Interrupt Handlers
//Must clear pending bit!
//***************************************************
void handler_ext_int_0(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_1(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_2(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_3(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_4(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_5(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_6(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_7(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_8(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_9(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_10(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_11(void) __attribute__ ((interrupt ("IRQ")));


void handler_ext_int_0(void){ *((volatile uint32_t *) 0xE000E280) = 0x1; }
void handler_ext_int_1(void){ *((volatile uint32_t *) 0xE000E280) = 0x2; }
void handler_ext_int_2(void){ *((volatile uint32_t *) 0xE000E280) = 0x4; }
void handler_ext_int_3(void){ *((volatile uint32_t *) 0xE000E280) = 0x8; }
void handler_ext_int_4(void){ *((volatile uint32_t *) 0xE000E280) = 0x10; }
void handler_ext_int_5(void){ *((volatile uint32_t *) 0xE000E280) = 0x20; }
void handler_ext_int_6(void){ *((volatile uint32_t *) 0xE000E280) = 0x40; }
void handler_ext_int_7(void){ *((volatile uint32_t *) 0xE000E280) = 0x80; }
void handler_ext_int_8(void){ *((volatile uint32_t *) 0xE000E280) = 0x100; }
void handler_ext_int_9(void){
	*((volatile uint32_t *) 0xE000E280) = 0x200;
	*((volatile uint32_t *) 0xA8000000) = 0x0;
}
void handler_ext_int_10(void){ *((volatile uint32_t *) 0xE000E280) = 0x400; }
void handler_ext_int_11(void){ *((volatile uint32_t *) 0xE000E280) = 0x800; }

//***************************************************
// Subfunctions
//***************************************************



void GPIO_ctrl(volatile uint32_t data){ 
	GPIO_WRITE = data;
	*GPIO = GPIO_WRITE; 
}
void GPIO_set(uint32_t loc){ 
	GPIO_WRITE = (GPIO_WRITE | (1 << loc));
	*GPIO = GPIO_WRITE;
}
void GPIO_kill(uint32_t loc){ 
	GPIO_WRITE = ~( (~GPIO_WRITE) | (1 << loc));
	*GPIO = GPIO_WRITE;
}

void initSPIMode(){
	*GPIODIR = 0x00000401;
//	*GPIODIR = 0x00000100;								// MISO at 0x10
	GPIO_set(SPI_SS_TIMER);	// Chip select to 1
	GPIO_set(SPI_SS_SRAM);	// Chip select to 1
	GPIO_set(SPI_SS_FLASH);	// Chip select to 1
	GPIO_set(SPI_SS_AFE);	// Chip select to 1
	GPIO_kill(SPI_CLK);	// Setting clk to 0
	GPIO_kill(SPI_0);	// Setting MO to 0
	GPIO_set(SPI_3);	// Setting Hold to 1
	delay(SPI_DELAY*2);
}

void writeSPI(volatile uint32_t data, volatile uint32_t writeBit){
	volatile uint32_t i;
	volatile uint32_t pos;
	pos = 1 << (writeBit-1);

	for(i = 0; i < writeBit; i++){
		if((data & pos)) GPIO_set(SPI_MOSI);
		else GPIO_kill(SPI_MOSI);
		delay(SPI_DELAY);
		GPIO_set(SPI_CLK);
		delay(SPI_DELAY);
		delay(SPI_DELAY);
		GPIO_kill(SPI_CLK);
		delay(SPI_DELAY);
		pos = pos >> 1;
	}
	GPIO_kill(SPI_MOSI);
	delay(SPI_DELAY);
}

void writeSPIFlash(volatile uint32_t data, volatile uint32_t writeBit){
	volatile uint32_t i;
	volatile uint32_t pos;
	pos = 1 << (writeBit-1);

	for(i = 0; i < writeBit; i++){
		if((data & pos)) GPIO_set(SPI_MOSI_FLASH);
		else GPIO_kill(SPI_MOSI_FLASH);
		delay(SPI_DELAY);
		GPIO_set(SPI_CLK_FLASH);
		delay(SPI_DELAY);
		delay(SPI_DELAY);
		GPIO_kill(SPI_CLK_FLASH);
		delay(SPI_DELAY);
		pos = pos >> 1;
	}
	GPIO_kill(SPI_MOSI_FLASH);
	delay(SPI_DELAY);
}


void endSPI(volatile uint32_t SPI_NUM){ 
	if(SPI_NUM == 0) GPIO_set(SPI_SS_TIMER);
	else if(SPI_NUM == 1) GPIO_set(SPI_SS_SRAM);
	else if(SPI_NUM == 2) GPIO_set(SPI_SS_FLASH);
	else if(SPI_NUM == 3) GPIO_set(SPI_SS_AFE);
}

void initSQIModeWrite(){
	*GPIODIR = 0x00000400;
	GPIO_set(SPI_SS_TIMER);	// Chip select to 1
	GPIO_set(SPI_SS_SRAM);	// Chip select to 1
	GPIO_set(SPI_SS_FLASH);	// Chip select to 1
	GPIO_set(SPI_SS_AFE);	// Chip select to 1
	GPIO_kill(SPI_CLK);	// Setting clk to 0
	GPIO_kill(SPI_0);	// 
	GPIO_kill(SPI_1);	//
	GPIO_kill(SPI_2);	// 
	GPIO_kill(SPI_3);	// 
	delay(SPI_DELAY*2);
}

void startSPI(volatile uint32_t SPI_NUM){ 
	if(SPI_NUM == 0){
		GPIO_kill(SPI_SS_TIMER);
	}
	else if(SPI_NUM == 1){
		GPIO_kill(SPI_SS_SRAM);
		GPIO_set(SPI_3); // Setting Hold to 1
	}
	else if(SPI_NUM == 2){
		GPIO_kill(SPI_SS_FLASH);
		GPIO_set(SPI_3); // Setting Hold to 1
	}
	else if(SPI_NUM == 3){
		GPIO_kill(SPI_SS_AFE);
	}
	delay(SPI_DELAY*2);
}

void startSQI(volatile uint32_t SPI_NUM){ 
	if(SPI_NUM == 0) GPIO_kill(SPI_SS_TIMER);
	else if(SPI_NUM == 1) GPIO_kill(SPI_SS_SRAM);
	else if(SPI_NUM == 2) GPIO_kill(SPI_SS_FLASH);
	delay(SPI_DELAY*2);
}

void writeSQI(volatile uint32_t data, volatile uint32_t writeBit){
	volatile uint32_t i;
	volatile uint32_t pos;
	pos = 1 << (writeBit-1);

	for(i = 0; i < writeBit/4; i++){
		if((data & pos)) GPIO_set(SPI_3);
		else GPIO_kill(SPI_3);
		pos = pos >> 1;
		if((data & pos)) GPIO_set(SPI_2);
		else GPIO_kill(SPI_2);
		pos = pos >> 1;
		if((data & pos)) GPIO_set(SPI_1);
		else GPIO_kill(SPI_1);
		pos = pos >> 1;
		if((data & pos)) GPIO_set(SPI_0);
		else GPIO_kill(SPI_0);
		pos = pos >> 1;
		delay(SPI_DELAY);
		GPIO_set(SPI_CLK);
		delay(SPI_DELAY);
		delay(SPI_DELAY);
		GPIO_kill(SPI_CLK);
		delay(SPI_DELAY);
	}
	GPIO_kill(SPI_0);
	GPIO_kill(SPI_1);
	GPIO_kill(SPI_2);
	GPIO_kill(SPI_3);
	delay(SPI_DELAY);
}



uint32_t readSPI(volatile uint32_t readBit){
	volatile uint32_t i;
	volatile uint32_t readData;
	readData = 0x00000000;

	for(i = 0; i < readBit; i++){
		GPIO_set(SPI_CLK);
		delay(SPI_DELAY);
		if(*GPIO & 1) readData = readData | (1 << (readBit-i-1));	//Flash MISO 11
//		if(*GPIO & 256) readData = readData | (1 << (readBit-i-1));	// MISO 0x100
		delay(SPI_DELAY);
		GPIO_kill(SPI_CLK);
		delay(SPI_DELAY);
		delay(SPI_DELAY);
	}
	return readData;
}

uint32_t readSPIFlash(volatile uint32_t readBit){
	volatile uint32_t i;
	volatile uint32_t readData;
	readData = 0x00000000;

	for(i = 0; i < readBit; i++){
		GPIO_set(SPI_CLK_FLASH);
		delay(SPI_DELAY);
		if(*GPIO & 1024) readData = readData | (1 << (readBit-i-1));
		delay(SPI_DELAY);
		GPIO_kill(SPI_CLK_FLASH);
		delay(SPI_DELAY);
		delay(SPI_DELAY);
	}
	return readData;
}
uint32_t readSQI(volatile uint32_t readBit){
	volatile uint32_t i;
	volatile uint32_t readData;
	readData = 0x00000000;

	for(i = 0; i < readBit; i=i+4){
		GPIO_set(SPI_CLK);
		delay(SPI_DELAY);
		if(*GPIO & 16) readData = readData | (1 << (readBit-i-1));		//Change here to meet the board layout
		if(*GPIO & 4) readData = readData | (1 << (readBit-i-2));
		if(*GPIO & 1) readData = readData | (1 << (readBit-i-3));
		if(*GPIO & 2) readData = readData | (1 << (readBit-i-4));
		delay(SPI_DELAY);
		GPIO_kill(SPI_CLK);
		delay(SPI_DELAY);
		delay(SPI_DELAY);
	}
	return readData;
}

uint32_t readWriteSPI(volatile uint32_t data, volatile uint32_t writeBit){
	volatile uint32_t i;
	volatile uint32_t pos;
	volatile uint32_t readData;
	readData = 0x00000000;
	pos = 1 << (writeBit-1);

	for(i = 0; i < writeBit; i++){
		if((data & pos)) GPIO_set(SPI_MOSI);
		else GPIO_kill(SPI_MOSI);
		delay(SPI_DELAY);
		GPIO_set(SPI_CLK);
		delay(SPI_DELAY);
		delay(SPI_DELAY);
		GPIO_kill(SPI_CLK);
		delay(SPI_DELAY);
		if(*GPIO & 1) readData = readData | (1 << (writeBit-i-1));
		pos = pos >> 1;
	}
//	GPIO_kill(SPI_MOSI);
	return readData;	
}

uint32_t readRDMR(){
	volatile uint32_t readData;
	readData = 0;
	startSPI(1);
	writeSPI(SRAM_RDMR, 8);
	readData = readSPI(8);
	endSPI(1);
	return readData;
}

void setWRMR(){
	startSPI(1);
	writeSPI(SRAM_WRMR, 8);
	writeSPI(0x40, 8);		// SeqMode: 0x40, ByteMode: 0x00, PageMode: 0x80
	endSPI(1);
}

void startSRAM(volatile uint32_t instSRAM, volatile uint32_t addrSRAM){
	startSPI(1);
	writeSPI(instSRAM, 8);
	writeSPI(addrSRAM, 24);
}
void endSRAM() {endSPI(1);}

void startSRAMSQI(volatile uint32_t instSRAM, volatile uint32_t addrSRAM){
	startSQI(1);
	writeSQI(instSRAM, 8);
	writeSQI(addrSRAM, 24);
}

void toSQI(){
	startSPI(1);
	writeSPI(SRAM_EQIO, 8);
	endSPI(1);
}

void toSPI(){
	startSQI(1);
	writeSQI(SRAM_RSTIO, 8);
	endSPI(1);
}

uint32_t readFlashStat(){
	volatile uint32_t readData;
	readData = 0;
	startSPI(2);
	writeSPIFlash(FLASH_READ_STAT, 8);
	readData = readSPI(8);
	endSPI(2);
	return readData;
}


uint32_t readFlashIden(){
	volatile uint32_t readData;
	readData = 0;
	startSPI(2);
	writeSPIFlash(FLASH_READ_IDEN, 8);
	readData = readSPI(20);
	endSPI(2);
	return readData;
}

void initialize(){
	volatile uint32_t i;
	volatile uint32_t time;
	volatile uint32_t date;

	enumerated = 0xABCDEF01;
	write_mbus_message(0x13,0xEEEEEEEE);
	GPIO_WRITE = 0;
	//Truning off the watch dog
	*((volatile uint32_t *) 0xA5000000) = 0;

//***********************************************************************************
// TIMER write & read test
//***********************************************************************************  
/*
	initSPIMode();
	time = 0;
	date = 0;
	timerAddr = 0;
	write_mbus_message(0x13, 0xDDDDDDDD);
	for(i = 0; i < 4; i++){
		startSPI(0);	// kill chip select of Timer
		writeSPI((0x0000007F&timerAddr), 8);	// send offset address starting with 0 (read mode)
		readData = readSPI(8);			// read data;
		endSPI(0);
		delay(100);
		time = time | (readData << (8*i));
		timerAddr = timerAddr + 1;
	}
	write_mbus_message(0x13, time);
	for(i = 0; i < 4; i++){
		startSPI(0);	// kill chip select of Timer
		writeSPI((0x0000007F&timerAddr), 8);	// send offset address starting with 0 (read mode)
		readData = readSPI(8);			// read data;
		endSPI(0);
		delay(100);
		date = date | (readData << (8*i));
		timerAddr = timerAddr + 1;
	}
	write_mbus_message(0x13, date);
	write_mbus_message(0x13, 0xDDDDDDDD);
	delay(5000);
*/

}

//***************************************************************************************
// MAIN function starts here             
//***************************************************************************************

int main() {
	volatile uint32_t  writeData;
	volatile uint32_t  readData;
	volatile uint32_t  sramInst;
	volatile uint32_t  sramAddr;
	volatile uint32_t  flashAddr;
	volatile uint32_t  timerAddr;
	volatile uint32_t  i;
	volatile uint32_t  time;
	volatile uint32_t  date;

	//Clear All Pending Interrupts
	*((volatile uint32_t *) 0xE000E280) = 0x3FF;
	//Enable Interrupts
//	*((volatile uint32_t *) 0xE000E100) = 0x07F;	// All timer ignored
//	*((volatile uint32_t *) 0xE000E100) = 0x3FF;	// All interrupt enable
	*((volatile uint32_t *) 0xE000E100) = 0;	// All interrupt disable
	//initialize	
	if( enumerated != 0xABCDEF01 ) initialize();
	// MEM&CORE CLK /16
	*((volatile uint32_t *) 0xA2000008) = 0x120F903;	//Isolation disable
	// PMU control
	*((volatile uint32_t *) 0xA200000C) = 0x00003060;	//Isolation disable

//	writeData = 0x00000000;
	writeData = 0x00000000;
	readData  = 0x00000000;
	sramInst  = 0x00000000;
	sramAddr  = 0x00000000;
	flashAddr = 0x000F0000;
	timerAddr = 0x00000000;
	time = 0;
	date = 0;

	
//
	initSPIMode();
	delay(5000);


//***********************************************************************************
// FLASH write & read test
//***********************************************************************************  
	//Flash write enable
	startSPI(2);
	writeSPIFlash(FLASH_WRITE_EN, 8);
	endSPI(2);

	delay(5000);
	readData = readFlashStat();
//	readData = readFlashIden();

//	write_mbus_message(0x13, 0xBEEF);
//	write_mbus_message(0x13, readData);
//	write_mbus_message(0x13, 0xBEEF);

	startSPI(2);				// Flash SPI start
	writeSPIFlash(FLASH_PAGE_PROGRAM, 8);	// Command to initiate the Page program
	writeSPIFlash(flashAddr, 24);		// Address starts from 0x000000
	for(i = 0; i < TEST_ITR; i++){
		writeSPIFlash(writeData, 8);
		writeData = writeData + 1;
//		write_mbus_message(0x13, writeData);
//		delay(5000);
	}
	endSPI(2);

	write_mbus_message(0x13,0xFFFFFFFF);
	delay(5000);
	startSPI(2);
	writeSPIFlash(FLASH_READ_DATA_BYTE, 8);
	writeSPIFlash(flashAddr, 24);		// Address starts from 0x000000
	for(i = 0; i< TEST_ITR; i++){
		readData = readSPIFlash(8);
		write_mbus_message(0x13, readData);
		delay(5000);
	}
	endSPI(2);

//	set_wakeup_timer(SLEEP_TIME, 0x1, 0x0);
//	*((volatile uint32_t *) 0xA2000014) = 0x01;
//	sleep();
//	while(1);
//***********************************************************************************
// SRAM write & read test
//***********************************************************************************  
//	initSPIMode();
	writeData = 0x00000000;
	delay(5000);
	sramInst  = SRAM_WRITE;
	GPIO_kill(MUX_SEL);		// SRAM access from M3

	setWRMR();
	delay(5000);
	
	readData = readRDMR();
	delay(5000);

//	write_mbus_message(0x13, 0xDEAD);
	write_mbus_message(0x13, readData);
//	write_mbus_message(0x13, 0xDEAD);
//SRAM write
	startSRAM(sramInst, sramAddr);
	for(i = 0; i < TEST_ITR; i++){
		writeSPI(writeData, 8);
		writeData = writeData + 1;
//		write_mbus_message(0x13, writeData);
//		delay(10000);
	}
	endSRAM();
	delay(1000);

//SRAM read	
	write_mbus_message(0x13,0xCCCCCCCC);
	delay(10000);
	sramInst = SRAM_READ;
	startSRAM(sramInst, sramAddr);
	for(i = 0; i < TEST_ITR; i++){
		readData = readSPI(8);
		write_mbus_message(0x13, readData);
		delay(10000);
	}
	endSRAM();
	delay(1000);

	write_mbus_message(0x13,0xCCCCCCCC);
	delay(10000);
	startSRAM(sramInst, sramAddr);
	for(i = 0; i < TEST_ITR; i++){
		readData = readSPI(8);
		write_mbus_message(0x13, readData);
		delay(10000);
	}
	endSRAM();
	delay(1000);
/*
//SRAM change to SQI
	toSQI();
	initSQIModeWrite();

//SRAM write SQI	
	sramInst = SRAM_WRITE;
	startSRAMSQI(sramInst, sramAddr);
	delay(1000);
	GPIO_set(MUX_SEL);
*/

/*
	for(i = 0; i < TEST_ITR; i++){
		writeSQI(writeData, 8);
		writeData = writeData + 1;
	}
*/
/*
	GPIO_kill(MUX_SEL);
	delay(1000);
	endSRAM();
	delay(1000);
//SRAM read SQI	
	sramInst = SRAM_READ;
	startSRAMSQI(sramInst, sramAddr);

	*GPIODIR = 0x0017;

	for(i = 0; i < TEST_ITR; i++){
		readData = readSQI(8);
		write_mbus_message(0x13, readData);
		delay(10000);
	}
	endSRAM();
	delay(1000);
	
	*GPIODIR = 0x0401;
	toSPI();
	initSPIMode();
*/


//***********************************************************************************
// TIMER write & read test
//***********************************************************************************  
	initSPIMode();
	time = 0;
	date = 0;
	timerAddr = 0;
	write_mbus_message(0x13, 0xDDDDDDDD);
	while(1){
	for(i = 0; i < 4; i++){
		startSPI(0);	// kill chip select of Timer
		writeSPI((0x0000007F&timerAddr), 8);	// send offset address starting with 0 (read mode)
		readData = readSPI(8);			// read data;
		endSPI(0);
		delay(100);
		time = time | (readData << (8*i));
		timerAddr = timerAddr + 1;
	}
	write_mbus_message(0x13, time);
	for(i = 0; i < 4; i++){
		startSPI(0);	// kill chip select of Timer
		writeSPI((0x0000007F&timerAddr), 8);	// send offset address starting with 0 (read mode)
		readData = readSPI(8);			// read data;
		endSPI(0);
		delay(100);
		date = date | (readData << (8*i));
		timerAddr = timerAddr + 1;
	}

	write_mbus_message(0x13, date);
	delay(10000);
	}
	// Reset wakeup counter
	// This is required to go back to sleep!!
	set_wakeup_timer(SLEEP_TIME, 0x1, 0x0);
	*((volatile uint32_t *) 0xA2000014) = 0x01;
	sleep();
	while(1);
}
