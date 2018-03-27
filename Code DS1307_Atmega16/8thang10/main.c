/*
* 8thang10.c
*
* Created: 10/8/2017 9:33:18 AM
* Author : MAIPHU
*/
#define F_CPU 11059200UL     //Neu tren proteus khong chay thi bo lenh nay di 
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "myLCD.h"
#include <avr/interrupt.h>

#define  SecondAddress 0x00
#define  WriteMode 0xD0
#define  ReadMode 0xD1
#define LED1 6
#define LED2 7
uint8_t c=0,I_RH,D_RH,I_Temp,D_Temp,CheckSum;
#define DHT11_PIN 2

char i;
char dis[5];
int adc_nhietdo;
uint8_t NhietDoMax = 25;
uint8_t DoamMax = 80;

typedef struct {
	uint8_t gio;
	uint8_t phut;
	uint8_t giay;
	uint8_t ngay;
	uint8_t thang;
	uint8_t nam;
	uint8_t thu;
} _rtc;
_rtc rtc;

void i2c_init();
void i2c_start();
void i2c_write(uint8_t data);
void i2c_stop();
uint8_t  i2c_read(uint8_t ackOption);
void read_time();

UART_init();
void UART_Write( unsigned char data );
void UART_Write_Text(unsigned char* str);
unsigned char UART_Read();
void UART_connect();
void UART_Display();

void ShowMainMenu(uint8_t sgiay,uint8_t sphut, uint8_t sgio,uint8_t sthu,uint8_t sngay, uint8_t sthang,uint8_t snam);
void SetTime(uint8_t cgiay,uint8_t cphut,uint8_t cgio,uint8_t cthu,uint8_t cngay, uint8_t cthang,uint8_t cnam);
void SetDate(uint8_t cgiay,uint8_t cphut,uint8_t cgio,uint8_t cthu,uint8_t cngay, uint8_t cthang,uint8_t cnam);
void SetAlarm(uint8_t cgiay,uint8_t cphut,uint8_t cgio,uint8_t cthu,uint8_t cngay, uint8_t cthang,uint8_t cnam);
int doihexsangnguyen(uint8_t chuc,uint8_t donvi);
uint8_t doinguyensanghex(int songuyen);
void displaytime(int tnhietdo,uint8_t tgiay,uint8_t tphut,uint8_t tgio,uint8_t tthu,uint8_t tngay,uint8_t tthang,uint8_t tnam);

void SetTemp();
void SetHumi();

void init_adc();
int read_adc(char channel);
void TEMP();

void Request();
void Response();
uint8_t Receive_data();
void DHT11();

void _DieuKhien(uint8_t TenTai, int TrangThai);
int main(void)
{
	DDRC |= (1<<LED1)|(1<<LED2);//0xff;
	DDRD = 0x00;                        
	PORTD = 0xf0; 

	init_adc();
	init_LCD();
	clr_LCD();
	move_LCD(1,1);
	print_LCD("Bui Quoc Anh");
	move_LCD(2,1);
	print_LCD("BTL Ghep Noi");
	_delay_ms(2000);
	clr_LCD();
	i2c_init();
	UART_init();
	_delay_ms(100);
	//sei();
	UART_connect();
	_delay_ms(100);
	/* Replace with your application code */
	while (1)
	{
		DHT11();
		TEMP();
		read_time();		
		displaytime(adc_nhietdo,rtc.giay,rtc.phut,rtc.gio,rtc.thu,rtc.ngay,rtc.thang,rtc.nam);
		UART_Display();		
		_delay_ms(500);
	}
}

void i2c_init()
{
	TWSR=0x00;
	TWBR=0x46;
	TWCR=0x04;
}

void i2c_start()
{
	TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while((TWCR &(1<<TWINT))==0);
	//while (!(TWCR & (1<<TWINT)));
}

void i2c_write(uint8_t data)
{
	TWDR=data;
	TWCR = ((1<< TWINT) | (1<<TWEN));
	while (!(TWCR & (1 <<TWINT)));
	//while((TWCR &(1<<TWINT))==0);
}

void i2c_stop()
{
	TWCR = ((1<< TWINT) | (1<<TWEN) | (1<<TWSTO));
	_delay_ms(5);
}


uint8_t  i2c_read(uint8_t ackOption)
{
	TWCR = ((1<< TWINT) | (1<<TWEN) | (ackOption<<TWEA));
	while ( !(TWCR & (1 <<TWINT)));
	return TWDR;
}

void read_time(){
	i2c_start();
	i2c_write(WriteMode);//0xD0//1101000/0 SLA + 0(bit write) 
	i2c_write(SecondAddress);//SecondAddress
	i2c_stop();
	i2c_start();
	i2c_write(ReadMode);//0xD1//1101000/1 SLA + 1(bit read) 

	//_rtc rtc;
	rtc.giay  = i2c_read(1);
	rtc.phut = i2c_read(1);
	rtc.gio  = i2c_read(1);
	rtc.thu = i2c_read(1);
	rtc.ngay = i2c_read(1);
	rtc.thang  = i2c_read(1);
	rtc.nam = i2c_read(0);
	/*Cu sau moi byte nhan duoc se gui ACK h?c NOT ACK*/
	i2c_stop();
}

UART_init(){
	UCSRA=0x00;//chon che do
	UCSRB=(1<<RXEN)|(1<<TXEN);//0x18;truyen nhan
	UCSRC=(1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1);//0x86;khung truyen
	UBRRH=0x00;// toc do truy?n baurate 
	UBRRL=5;//71;//0x05;
}
void UART_Write( unsigned char data )
{
	UDR = data;//thanh ghi du lieu,chua 8 bit truyen va nhan 
	while (!(UCSRA & (1<<UDRE)));// Neu bit nay bang 1 >> UDR trong
}

void UART_Write_Text(unsigned char* str)
{
	int i=0;
	while (str[i]!=0)
	{
		UART_Write(str[i]);
		i++;
	}
}

unsigned char UART_Read()
{
	while (!(UCSRA & (1 << RXC))); // bao nhan ket thuc
	return(UDR);
}

void UART_connect()
{
	UART_Write_Text("AT\r\n");
	_delay_ms(20);
	
	UART_Write_Text("AT+CWMODE=1\r\n");

	_delay_ms(20);
	UART_Write_Text("AT+CWJAP=\"TP-LINK_6B50\",\"41633465\"\r\n");
	//UART_Write_Text("AT+CWJAP=\"DiemHen\",\"hoilamgi\"\r\n");
	//UART_Write_Text("AT+CWJAP=\"AndroidAPE52E\",\"whatislove\"\r\n");

	_delay_ms(20);
}

void UART_Display()
{
	char str[150];
	char _guidodai[30];
	UART_Write_Text("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80\r\n");
	_delay_ms(100);
	memset(str,0,150);
	sprintf(str,"GET /update?key=SIM0PODO002EFZT1&field1=%d&field2=%d\r\n",adc_nhietdo,I_RH);
	memset(_guidodai, 0,30);
	sprintf(_guidodai, "AT+CIPSEND=%d\r\n", (strlen(str)+2));
	UART_Write_Text(_guidodai);
	_delay_ms(50);
	UART_Write_Text(str);
	_delay_ms(100);
}

void init_adc(){
	DDRA = 0x00;
	ADCSRA = 0x87;//10000111(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);//bat ADC va set xung nhip
	ADMUX = 0x40;//01000000// chon dien ap tham chieu 5V
}
int read_adc(char channel)
{
	ADMUX = 0x40 | (channel & 0x07);
	ADCSRA |= (1<<ADSC);//bat ?au qua trinh chuyen doi
	while (!(ADCSRA & (1<<ADIF)));//sau khi chuyen doi , ADIF duoc set len 1
	ADCSRA |= (1<<ADIF);
	_delay_ms(1);
	return ADCW;//ADCL va ADCH
}
void TEMP(){
	adc_nhietdo = (int)read_adc(0) * 0.488;
	_delay_ms(5);
	if(adc_nhietdo>=NhietDoMax)
	{
		_DieuKhien(1,1);
	}
	else
	{
		_DieuKhien(1,0);
	}
}
void DHT11(){
	Request();	
	Response();	
	I_RH=Receive_data();
	D_RH=Receive_data();	
	I_Temp=Receive_data();	
	D_Temp=Receive_data();	
	CheckSum=Receive_data();
	if(I_RH>=DoamMax)
	{
		_DieuKhien(2,1);
	}
	else
	{
		_DieuKhien(2,0);
	}
}

void _DieuKhien(uint8_t TenTai, int TrangThai)
{
	switch(TenTai)
	{
		case 1:
		{
			if (TrangThai == 1)
			{
				PORTC &= ~(1<<LED1);
			}
			
			else{
				PORTC |= (1<<LED1);
			}
			
			break;
		}
		case 2:
		{
			if (TrangThai == 1)
			{
				PORTC &= ~(1<<LED2);
			}
			
			else{
				PORTC |= (1<<LED2);
			}
			break;
		}
		default:
		{
			break;
		}
	}
}

void Request()				
{
	DDRC |= (1<<DHT11_PIN);
	PORTC &= ~(1<<DHT11_PIN);	
	_delay_ms(20);		//cho it nhat 18ms
	PORTC |= (1<<DHT11_PIN);	
}

void Response()			
{
	DDRC &= ~(1<<DHT11_PIN);
	while(PINC & (1<<DHT11_PIN));
	while((PINC & (1<<DHT11_PIN))==0);
	while(PINC & (1<<DHT11_PIN));
}

uint8_t Receive_data()	
{
	for (int q=0; q<8; q++)
	{
		while((PINC & (1<<DHT11_PIN)) == 0);  /* kiem tra nhan bit 0 or 1 */
		_delay_us(30);
		if(PINC & (1<<DHT11_PIN))/* Neu lon hon 30ms */
		c = (c<<1)|(0x01);	/* la muc cao*/
		else			/* con khong la thap */
		c = (c<<1);
		while(PINC & (1<<DHT11_PIN));
	}
	return c;
}

void displaytime(int tnhietdo,uint8_t tgiay,uint8_t tphut,uint8_t tgio,uint8_t tthu,uint8_t tngay,uint8_t tthang,uint8_t tnam){
	char thu[7][4]={"SAT","SUN","MON","TUE","WED","THU","FRI"};
		
	sprintf(dis, "%2x",tgio);move_LCD(1,1); print_LCD(dis);
	move_LCD(1 , 3);print_LCD(":");
	sprintf(dis, "%2x",tphut);move_LCD(1 , 4); print_LCD(dis);
	move_LCD(1,6); putChar_LCD(':');sprintf(dis,"%2x",tgiay);
	move_LCD(1,7);	print_LCD(dis);

	move_LCD(1,10);
	print_LCD(thu[tthu]);
	move_LCD(1,14);
	sprintf(dis,"%2d",I_RH);
	print_LCD(dis);move_LCD(1,16);putChar_LCD(0x25);
	
	sprintf(dis, "%2x",tngay);move_LCD(2,1); print_LCD(dis);
	move_LCD(2,3);print_LCD("/");
	sprintf(dis, "%2x",tthang);move_LCD(2,4); print_LCD(dis);
	move_LCD(2,6); print_LCD("/");
	move_LCD(2,7); print_LCD("20");
	sprintf(dis, "%2x",tnam);	move_LCD(2,9);	print_LCD(dis);
	sprintf(dis, "%2d",tnhietdo);move_LCD(2,12); print_LCD(dis);
	move_LCD(2,14); putChar_LCD(0xdf);
	move_LCD(2,15); print_LCD("C");
	

	for(i=0; i<60; i++)
	{
		if(bit_is_clear(PIND,4) != 0)
		{
			// Main Menu
			_delay_ms(250);
			clr_LCD();
			ShowMainMenu(tgiay,tphut,tgio,tthu,tngay,tthang,tnam);
			break;
		}
		_delay_ms(1);
	}
}
void ShowMainMenu(uint8_t sgiay,uint8_t sphut, uint8_t sgio,uint8_t sthu,uint8_t sngay, uint8_t sthang,uint8_t snam)
{
	// Main Menu
	char menu[5][11]={ "Set Time", "Set Date", "Alarm Time","Set Temp","Set Humi"};
	char sel=0;

	//	_delay_ms(1);

	while(1)
	{
		move_LCD(1,5);
		print_LCD("Main Menu");
		move_LCD(2, 1);
		print_LCD("<");
		move_LCD(2, 16);
		print_LCD(">");
		move_LCD(2, 5);
		print_LCD(menu[sel]);

		if(bit_is_clear(PIND,6) != 0)
		{
			if(sel == 0)
			sel=4;
			else
			sel--;
			_delay_ms(250);
			clr_LCD();
		}

		else if(bit_is_clear(PIND,5) != 0)
		{
			if(sel == 4)
			sel=0;
			else
			sel++;
			_delay_ms(250);
			clr_LCD();
		}

		else if(bit_is_clear(PIND,4) != 0)
		{
			//while(bit_is_clear(PIND,4) != 0);
			//_delay_ms(250);
			_delay_ms(200);
			clr_LCD();
			switch (sel)
			{
				case 0:  SetTime(sgiay,sphut,sgio,sthu,sngay,sthang,snam);
				break;
				case 1:  SetDate(sgiay,sphut,sgio,sthu,sngay,sthang,snam);
				break;
				case 2:  SetAlarm(sgiay,sphut,sgio,sthu,sngay,sthang,snam);
				break;
				case 3:  SetTemp();
				break;
				case 4:  SetHumi();
				break;
			}
			break;
		}
		_delay_ms(150);
	}
}

void SetTemp(){
	clr_LCD();
	while(1){
		move_LCD(1,1);
		print_LCD("Set Temperature ");
		move_LCD(2,4);
		print_LCD("MAX :");
		
		sprintf(dis,"%2d",NhietDoMax);
		move_LCD(2,10);
		print_LCD(dis);
		
		if(bit_is_clear(PIND,6) != 0)
		{
			while(bit_is_clear(PIND,6) != 0);
			NhietDoMax = NhietDoMax + 1;
			if(NhietDoMax > 60)
			{
				NhietDoMax = 35;
			}
		}
		else if(bit_is_clear(PIND,5) != 0)
		{
			while(bit_is_clear(PIND,5) != 0);
			NhietDoMax = NhietDoMax-1;
			if(NhietDoMax < 10)
			{
				NhietDoMax = 35;
			}
		}
		else if(bit_is_clear(PIND,4) != 0)
		{
			_delay_ms(1000);
			break;
		}
	}
	read_time();
	clr_LCD();
	displaytime(adc_nhietdo,rtc.giay,rtc.phut,rtc.gio,rtc.thu,rtc.ngay,rtc.thang,rtc.nam);
}
void SetHumi(){
	clr_LCD();
	while(1){
		move_LCD(1,1);
		print_LCD("Set Humidity  ");
		move_LCD(2,4);
		print_LCD("MAX :");
		
		sprintf(dis,"%2d",DoamMax);
		move_LCD(2,10);
		print_LCD(dis);
		
		if(bit_is_clear(PIND,6) != 0)
		{
			while(bit_is_clear(PIND,6) != 0);
			DoamMax = DoamMax + 1;
			if(DoamMax > 99)
			{
				DoamMax = 50;
			}
		}
		else if(bit_is_clear(PIND,5) != 0)
		{
			while(bit_is_clear(PIND,5) != 0);
			DoamMax = DoamMax - 1;
			if(DoamMax < 30)
			{
				DoamMax = 60;
			}
		}
		else if(bit_is_clear(PIND,4) != 0)
		{
			_delay_ms(1000);
			break;
		}
	}
	read_time();
	clr_LCD();
	displaytime(adc_nhietdo,rtc.giay,rtc.phut,rtc.gio,rtc.thu,rtc.ngay,rtc.thang,rtc.nam);
}

int doihexsangnguyen(uint8_t chuc,uint8_t donvi){
	chuc = chuc>>4;
	donvi = (donvi<<4);
	donvi = donvi>>4;
	return (chuc * 10 + donvi);
}

uint8_t doinguyensanghex(int songuyen){
	uint8_t chuchex = songuyen/10;
	chuchex = chuchex<<4;
	uint8_t dvhex = songuyen%10;
	return (chuchex + dvhex);
}

void SetTime(uint8_t cgiay,uint8_t cphut,uint8_t cgio,uint8_t cthu,uint8_t cngay, uint8_t cthang,uint8_t cnam)
{
	static char am_pm=0;
	char sel=1;
	uint8_t _gio,_phut,_giay;
	_gio = cgio;
	_phut = cphut;
	_giay = cgiay;
	int stgio = doihexsangnguyen(cgio,_gio);
	int stphut = doihexsangnguyen(cphut,_phut);
	int stgiay = doihexsangnguyen(cgiay,_giay);

	clr_LCD();
	while(1)
	{
		char dis[5];
		sprintf(dis, "%2d",stgio);move_LCD(1,1); print_LCD(dis);
		move_LCD(1 , 3);putChar_LCD(':');
		sprintf(dis, "%2d",stphut);move_LCD(1 , 4); print_LCD(dis);
		move_LCD(1,6); putChar_LCD(':');sprintf(dis,"%2d",stgiay);
		move_LCD(1,7);	print_LCD(dis);

		if(am_pm)
		{
			move_LCD(1,10);
			print_LCD("OK");
		}
		else
		{
			move_LCD(1,10);
			print_LCD("OK");
		}
		move_LCD(2, sel);
		print_LCD("^^");

		if(bit_is_clear(PIND,6) != 0)
		{
			if(sel==1)
			{
				//Gio
				if(stgio>=23)
				{
					stgio=0;
				}
				else
				{
					stgio++;
				}
			}

			else if(sel==4)
			{
				//Phut
				if(stphut==59)
				{
					stphut=0;
				}
				else
				{
					stphut++;
				}
			}

			else if(sel==7)
			{
				//Giay
				if(stgiay==59)
				{
					stgiay=0;
				}
				else
				{
					stgiay++;
				}
			}

			else if(sel==10)
			{
				break;
			}
			_delay_ms(150);
		}
		else if(bit_is_clear(PIND,5) != 0)   //Gi?m
		{
			if(sel==1)
			{
				//Gio
				if(stgio==0)
				{
					stgio=23;
				}
				else
				{
					stgio--;
				}
			}

			else if(sel==4)
			{
				//Phut
				if(stphut==0)
				{
					stphut=59;
				}
				else
				{
					stphut--;
				}
			}

			else if(sel==7)
			{
				//Giay
				if(stgiay==0)
				{
					stgiay=59;
				}
				else
				{
					stgiay--;
				}
			}
			else if(sel==10)
			{
				break;
			}
			_delay_ms(150);
		}
		else if(bit_is_clear(PIND,4) != 0)
		{
			if(sel>=10)
			sel=1;
			else
			sel=sel+3;
			clr_LCD();
		}
		_delay_ms(150);
	}//ket thuc vong while
	
	
	uint8_t setgio = doinguyensanghex(stgio);
	uint8_t setphut = doinguyensanghex(stphut);
	uint8_t setgiay = doinguyensanghex(stgiay);

	
	
	i2c_start();
	i2c_write(WriteMode);
	i2c_write(SecondAddress);
	i2c_stop();
	i2c_start();
	i2c_write(ReadMode);

	//_rtc rtc;
	rtc.giay  = i2c_read(1);
	rtc.phut = i2c_read(1);
	rtc.gio  = i2c_read(1);
	rtc.thu = i2c_read(1);
	rtc.ngay = i2c_read(1);
	rtc.thang  = i2c_read(1);
	rtc.nam = i2c_read(0);

	i2c_stop();

	rtc.gio = setgio;
	rtc.phut = setphut;
	rtc.giay =  setgiay;


	i2c_start();
	i2c_write(WriteMode);
	i2c_write(SecondAddress);

	i2c_write(rtc.giay);
	i2c_write(rtc.phut);
	i2c_write(rtc.gio);
	i2c_write(rtc.thu);
	i2c_write(rtc.ngay);
	i2c_write(rtc.thang);
	i2c_write(rtc.nam);

	i2c_stop();
	clr_LCD();
	displaytime(adc_nhietdo,rtc.giay,rtc.phut,rtc.gio,rtc.thu,rtc.ngay,rtc.thang,rtc.nam);
}
void SetDate(uint8_t cgiay,uint8_t cphut,uint8_t cgio,uint8_t cthu,uint8_t cngay, uint8_t cthang,uint8_t cnam)
{
	char dis[5];
	char _day[7][4]={"SAT","SUN","MON","TUE","WED","THU","FRI"};
	unsigned char  sel =1;
	uint8_t _thu,_ngay,_thang,_nam;
	_thu = cthu;
	_ngay = cngay;
	_thang = cthang;
	_nam = cnam;
	int stthu = doihexsangnguyen(cthu,_thu);
	int stngay = doihexsangnguyen(cngay,_ngay);
	int stthang = doihexsangnguyen(cthang,_thang);
	int stnam = doihexsangnguyen(cnam,_nam);

	clr_LCD();

	while(1)
	{
		sprintf(dis,"%2d",stngay);move_LCD(1, 1);print_LCD(dis);
		move_LCD(1,3);putChar_LCD('/');
		sprintf(dis,"%2d",stthang);move_LCD(1, 4);print_LCD(dis);
		move_LCD(1,6);putChar_LCD('/');
		sprintf(dis,"%2d",stnam);move_LCD(1, 7);print_LCD(dis);
		move_LCD(1,10);print_LCD(_day[stthu]);
		move_LCD(1, 14);print_LCD("OK");
		move_LCD(2, sel);print_LCD("^^");
		
		//Tang
		if(bit_is_clear(PIND,6) != 0)
		{
			if(sel==1)
			{
				if(stngay==31)
				{
					stngay=1;
				}
				else
				{
					stngay++;
				}
			}

			else if(sel==4)
			{
				if(stthang==12)
				{
					stthang=1;
				}
				else
				{
					stthang++;
				}
			}

			else if(sel==7)
			{
				if(stnam==99)
				{
					stnam=11;
				}
				else
				{
					stnam++;
				}
			}

			else if(sel==10)
			{
				if(stthu >= 6)
				{
					stthu=0;
				}
				else
				{
					stthu++;
				}
			}
			else if(sel == 14)
			{
				//OK
				break;
			}
			_delay_ms(150);
		}

		// Giam
		else if(bit_is_clear(PIND,5) != 0)
		{
			if(sel==1)
			{
				if(stngay==1)
				{
					stngay=31;
				}
				else
				{
					stngay--;
				}
			}

			else if(sel==4)
			{
				if(stthang==1)
				{
					stthang=12;
				}
				else
				{
					stthang--;
				}
			}

			else if(sel==7)
			{
				if(stnam==11)
				{
					stnam=99;
				}
				else
				{
					stnam--;
				}
			}

			else if(sel==10)
			{
				if(stthu==0)
				{
					stthu=6;
				}
				else
				{
					stthu--;
				}
			}
			else if(sel == 14)
			{
				//OK
				break;
			}
			_delay_ms(150);
		}

		else if(bit_is_clear(PIND,4) != 0)
		{
			if(sel==14)
			sel=1;
			else if(sel==10)
			sel=14;
			else
			sel=sel+3;
			clr_LCD();
		}
		_delay_ms(150);
	}//ket thuc while
	if(stthang==4 | stthang==6 | stthang==9 | stthang==11)
	{
		if(stngay==31)
		stngay=30;
	}
	else if(stthang==2)
	{
		if(stngay==29 | stngay==30 | stngay==31)
		{
			for (int i = 12;i<99;i=i+4)
			{
				if(stnam==i){
					stngay=29;
					break;
				}
				else{
					stngay=28;
				}
				
			}
			
		}
	}
	uint8_t setthu = doinguyensanghex(stthu);
	uint8_t setngay = doinguyensanghex(stngay);
	uint8_t setthang = doinguyensanghex(stthang);
	uint8_t setnam = doinguyensanghex(stnam);

	//_rtc rtc;

	i2c_start();
	i2c_write(WriteMode);
	i2c_write(SecondAddress);
	i2c_stop();
	i2c_start();
	i2c_write(ReadMode);

	rtc.giay  = i2c_read(1);
	rtc.phut = i2c_read(1);
	rtc.gio  = i2c_read(1);
	rtc.thu = i2c_read(1);
	rtc.ngay = i2c_read(1);
	rtc.thang  = i2c_read(1);
	rtc.nam = i2c_read(0);

	i2c_stop();


	rtc.thu = setthu;
	rtc.ngay = setngay;
	rtc.thang = setthang;
	rtc.nam =  setnam;
	
	i2c_start();
	i2c_write(WriteMode);
	i2c_write(SecondAddress);

	i2c_write(rtc.giay);       //Second
	i2c_write(rtc.phut);       //Minute
	i2c_write(rtc.gio);       //Hour
	i2c_write(rtc.thu);       //Day
	i2c_write(rtc.ngay);       //Date
	i2c_write(rtc.thang);       //Month
	i2c_write(rtc.nam);

	i2c_stop();
	clr_LCD();
	displaytime(adc_nhietdo,rtc.giay,rtc.phut,rtc.gio,rtc.thu,rtc.ngay,rtc.thang,rtc.nam);
}
void SetAlarm(uint8_t cgiay,uint8_t cphut,uint8_t cgio,uint8_t cthu,uint8_t cngay, uint8_t cthang,uint8_t cnam){
	char dis[5];
	char sel = 1;
	uint8_t _gio,_phut,_giay;
	_gio = cgio;_phut = cphut;_giay = cgiay;
	int stgio = doihexsangnguyen(cgio,_gio);
	int stphut = doihexsangnguyen(cphut,_phut);
	int stgiay = doihexsangnguyen(cgiay,_giay);
	clr_LCD();
	while(1){
		sprintf(dis, "%2d",stgio);move_LCD(1,1); print_LCD(dis);
		move_LCD(1 , 3);putChar_LCD(':');
		sprintf(dis, "%2d",stphut);move_LCD(1 , 4); print_LCD(dis);
		//move_LCD(1,6); putChar_LCD(':');
		//sprintf(dis,"%2d",stgiay);move_LCD(1,7);print_LCD(dis);
		move_LCD(1,7);print_LCD("OK");
		move_LCD(2,sel);
		print_LCD("^^");
		
		if (bit_is_clear(PIND,6) != 0)
		{
			if (sel == 1)
			{
				if(stgio >=23){
					stgio = 0;
					}else{
					stgio++;
				}
			}
			else if (sel == 4)
			{
				if (stphut >=59)
				{
					stphut = 0;
					}else{
					stphut++;
				}
			}
			else if (sel >=7)
			{
				break;
			}
			_delay_ms(150);
		}
		else if (bit_is_clear(PIND,5) != 0)
		{
			if (sel == 1)
			{
				if(stgio == 0){
					stgio = 23;
					}else{
					stgio--;
				}
			}
			else if (sel == 4)
			{
				if (stphut == 0)
				{
					stphut = 59;
					}else{
					stphut--;
				}
			}
			else if (sel >=7)
			{
				break;
			}
			_delay_ms(150);
		}
		if (bit_is_clear(PIND,4) != 0)
		{
			if (sel == 7)
			{
				sel =1;
				}else {
				sel = sel+3;
			}
			clr_LCD();
		}
		_delay_ms(150);
	}//ket thuc while
	uint8_t setgio = doinguyensanghex(stgio);
	uint8_t setphut = doinguyensanghex(stphut);
	uint8_t setgiay = doinguyensanghex(stgiay);

	//_rtc rtc;
	int temp = 0;
	while(1){
		
		i2c_start();
		i2c_write(WriteMode);
		i2c_write(SecondAddress);
		i2c_stop();
		i2c_start();
		i2c_write(ReadMode);
		rtc.giay  = i2c_read(1);
		rtc.phut = i2c_read(1);
		rtc.gio  = i2c_read(1);
		rtc.thu = i2c_read(1);
		rtc.ngay = i2c_read(1);
		rtc.thang  = i2c_read(1);
		rtc.nam = i2c_read(0);

		i2c_stop();

		if((rtc.gio==setgio)&&(rtc.phut == setphut)){
			clr_LCD();
			sprintf(dis, "%2d",stgio);move_LCD(1,1); print_LCD(dis);
			move_LCD(1 , 3);putChar_LCD(':');
			sprintf(dis, "%2d",stphut);move_LCD(1,4); print_LCD(dis);
			move_LCD(2,1);print_LCD("Alarm!");
			_delay_ms(100);
			clr_LCD();
			_delay_ms(100);
			temp++;
		}
		else{
			//clr_LCD();
			displaytime(adc_nhietdo,rtc.giay,rtc.phut,rtc.gio,rtc.thu,rtc.ngay,rtc.thang,rtc.nam);
		}
		if (temp==20)
		{
			temp = 0;
			break;
		}
	}//ket thuc while
	clr_LCD();
	displaytime(adc_nhietdo,rtc.giay,rtc.phut,rtc.gio,rtc.thu,rtc.ngay,rtc.thang,rtc.nam);
}
