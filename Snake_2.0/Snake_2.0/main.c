/*
 * Final_snake.c
 *
 * Created: 20/09/2020 06:52:01 p. m.
 * Author : Alejandro
 */
#define F_CPU 1000000UL
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <stdio.h>
////////////////////////////////------------>CONSTANTES////////////////////////////////////////////////////
#define UP		    1 << PD3
#define DOWN		1 << PD2
#define LEFT		1 << PD7
#define RIGHT		1 << PD4
struct Position
{
	int8_t x; //3 bits would be enough to represent a width of 8 but oh well
	int8_t y;
}food, direction;
enum Cell
{
	EMPTY, SNAKE_LEFT, SNAKE_RIGHT, SNAKE_UP, SNAKE_DOWN, FOOD, BONUS
};
enum State
{
	GAME, MENU
};
struct Snake //89 bytes
{
	struct Position body[35]; //ideally it should be world.width * world.height. maybe malloc() it inside init_game() ?
	struct Position head, tail;
	uint8_t belly; //score
	uint8_t length;
}snake;
struct World
{
	enum Cell grid[35]; // Again, this should ideally be dynamically allocated depending on the size of the LED matrix
	uint8_t height;
	uint8_t width;
	enum State state;
}world;
volatile uint8_t button_g = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////------------->FUNCIONES////////////////////////////////////////////////////
void init_interrupts()
{
	EICRA |= (1 << ISC11)|(1 << ISC10)|(1 << ISC11)|(1 << ISC10);
	// Es necesario habilitar los pines, para que el microcontrolador
	// active la funci�n.
	EIMSK |= (1 << INT1)|(1 << INT0);
	sei();
	//Siempre activan en cambio de estado
	PCICR = (1<<PCIE0);
	PCMSK2 |= (1<<PCINT0);
	PCMSK0 |= (1<<PCINT7);
}
const struct Snake *get_snake(void)
{
	return &snake;
}
void handle_keypress(const struct Position *_direction)
{
	//ignore directions that don't make sense
	if(_direction->x == _direction->y)
	{
		return;
	}
	if(_direction->x != 0 && _direction->y != 0) //{-1, 1}, {1, -1}
	{
		return;
	}
	//ignore conflicting directions like making the snake go up when it's going down
	//or making it go left when it's moving right or vice versa
	if(_direction->x + direction.x == 0)
	{
		return;	
	}
	if(_direction->y + direction.y == 0)
	{
		return;
	}
	direction = *_direction;
}
void set_cell(const struct Position *position, enum Cell value) //ditto
{
	world.grid[position->y * world.width + position->x] = value;
}
struct Position direction_to_position(enum Cell direction)
{
	struct Position position = {0, 0};
	switch (direction)
	{
		case SNAKE_UP:
		position.y--;
		break;
		case SNAKE_DOWN:
		position.y++;
		break;
		case SNAKE_LEFT:
		position.x--;
		break;
		case SNAKE_RIGHT:
		position.x++;
		break;
		default:
		printf("Impossible : %u\n", direction);
		break;
	}
	return position;
}
enum Cell position_to_direction(const struct Position *position)
{
	if (position->x == -1)
	return SNAKE_LEFT;
	if (position->x == 1)
	return SNAKE_RIGHT;
	if (position->y == -1)
	return SNAKE_UP;
	return SNAKE_DOWN;
}
enum Cell cell_at(const struct Position *position) //probably belongs in a utils.h file
{
	return world.grid[position->y * world.width + position->x];
}
void place_food(void)
{
	//to do : the following algorithm will be slow when the snake occupies a large portion of the grid
	//it might even freeze the program
	do
	{
		food.x = rand() % world.width;
		food.y = rand() % world.height;
	}while(cell_at(&food) != EMPTY);
	set_cell(&food, FOOD);
}
void handle_wrapping(struct Position *position)
{
	if (position->x < 0)
	{
		position->x = world.width - 1;
	}
	else if (position->x >= world.width)
	{
		position->x = 0;
	}
	if (position->y < 0)
	{
		position->y = world.height - 1;
	}
	else if (position->y >= world.height)
	{
		position->y = 0;
	}
}
void move_head(const struct Position *head)
{
	if (head->x == snake.tail.x && head->y == snake.tail.y)
	{
		struct Position new_tail = direction_to_position(cell_at(&snake.tail));
		struct Position tail = {.x = snake.tail.x + new_tail.x, .y = snake.tail.y + new_tail.y};
		handle_wrapping(&tail);
		snake.head = snake.tail;
		return;
	}
	enum Cell head_direction = position_to_direction(&direction);
	snake.head = *head;
	set_cell(&snake.head, head_direction);
}
void move_tail(void)
{
	struct Position tail = snake.tail;
	enum Cell tail_direction = cell_at(&snake.tail);
	switch (tail_direction)
	{
		case SNAKE_UP:
			tail.y--;
			break;
		case SNAKE_DOWN:
			tail.y++;
			break;
		case SNAKE_LEFT:
			tail.x--;
			break;
		case SNAKE_RIGHT:
			tail.x++;
			break;
		default:
			printf("Impossible : %u\n", tail_direction);
			break;
	}
	handle_wrapping(&tail);
	//for when the tail and the head are in the same position
	//eg. a horizontal snake that occupies 8 squares:
	if (snake.head.x == snake.tail.x && snake.head.y == snake.tail.y)
	{
		set_cell(&snake.tail, tail_direction);
	}
	else
	{
		set_cell(&snake.tail, EMPTY);
	}
	snake.tail = tail;
}
enum State get_state(void)
{
	return world.state;
}
void set_state(enum State state)
{
	world.state = state;
}
const struct World *get_world(void)
{
	return &world;
}
void eat_food(const struct Position *head, enum Cell food_type)
{
	move_head(head);
	snake.belly += food_type == FOOD ? 1 : 5;
	place_food();
	snake.length++;
}
void tick(void)
{
	set_cell(&snake.head, position_to_direction(&direction));
	struct Position head = 
	{
		.x = direction.x + snake.head.x,
		.y = direction.y + snake.head.y
	};
	handle_wrapping(&head);
	enum Cell cell = cell_at(&head);
	switch (cell)
	{
		case FOOD:
		case BONUS:
			eat_food(&head, cell);
			break;
		case EMPTY:
			move_head(&head);
			move_tail();
			break;
		default:
			if (head.x == snake.tail.x && head.y == snake.tail.y)
			{
				move_head(&head);
				move_tail();
				world.state = MENU;
			}
			else
			{
				world.state = MENU;
			}
			break;
	}
}
void init_game(void)
{
	srand(18979); //to do : write it to EEPROM and next time the program is ran, read it and increment it
	direction.x = 1;
	direction.y = 0;
	world.height = 5;
	world.width = 7;
	snake.belly = 0;
	snake.length = 0;
	uint8_t c;
	for (c = 0; c < world.width * world.height; c++)
	{
		world.grid[c] = EMPTY;	
	}
	struct Position pos = {world.width / 2, world.height / 2};
	set_cell(&pos, SNAKE_LEFT);
	snake.head = pos;
	pos = (struct Position){world.width / 2 + 1, world.height / 2};
	set_cell(&pos, SNAKE_LEFT);
	pos = (struct Position){world.width / 2 + 2, world.height / 2};
	set_cell(&pos, SNAKE_LEFT);
	snake.tail = pos;
	place_food();
	handle_keypress(&direction);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////------------>INTERRUPCIONES/////////////////////////////////////////////
ISR(INT0_vect)
{
	button_g = 2;
}
ISR(INT1_vect)
{
	button_g = 1;
}
ISR(PCINT1_vect)
{
	button_g = 3;
}
ISR(PCINT0_vect)
{
	button_g = 3;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////------------->MATRIZ////////////////////////////////////////////////////
void init_matrix(void)
{
	DDRC = 0b00000001;
	DDRB = 0b01111111;
	DDRD = 0x00;
}
void write_column(uint8_t data, uint8_t column)
{
	//pulling a GND pin low and keeping all the others pulled high
	//this will cause that specific column of LEDs to turn on if their respective pins are pulled high
	//the image is drawn one column at a time, except it happens faster than what the human eye can keep up with
	PORTB = ~(1 << column);
	PORTC = data;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////-------------->PAD///////////////////////////////////////////////////////
void init_dpad(void)
{
	DDRD &= ~(UP | DOWN | LEFT | RIGHT);
}
bool button_pressed(uint8_t button)
{
	return PIND & button;
}
bool any_pressed(void)
{
	return PIND & (UP | DOWN | LEFT | RIGHT);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////------------------>RENDER GAME///////////////////////////////////////////////
void render(int repetitions)
{
	while(repetitions--) //it only displays correctly when multiplexing repeatedly
	{
		const struct World *world = get_world();
		uint8_t col, row;
		for(col = 0; col < world->width; col++)
		{
			uint8_t data = 0;
			for(row = 0; row < world->height; row++)
			{
				struct Position cell = {.x = col, .y = row};
				if(cell_at(&cell) != EMPTY)
				data |= 1 << row;
			}
			write_column(data, col);
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////------------------>MAIN//////////////////////////////////////////////////////
int main(void)
{
	init_game();
	set_state(GAME);
	struct Position direction = {.x = 0, .y = 1};
	init_matrix();
	init_interrupts();
	uint8_t tick_interval = 10;
	for(;;)
	{
		if(get_state() == GAME)
		{
			/* this should be put in a PCINT interrupt vector*/
			if(button_g == 1)
			{
				direction.x = 0;
				direction.y = -1;
			}
			else if(button_g == 2)
			{
				direction.x = 0;
				direction.y = 1;
			}
			else if(button_g == 3)
			{
				direction.x = -1;
				direction.y = 0;
			}
			else if(button_g == 4)
			{
				direction.x = 1;
				direction.y = 0;
			}
			handle_keypress(&direction);
			/* pin change interrupt end */
			//this should be put in a timer interrupt
			render(5);
			_delay_ms(8);
			if(tick_interval-- == 0)
			{
				tick_interval = 10;
				tick();
			}
		}
		else if(get_state() == MENU)
		{
			write_column(0xFF, 0);
			write_column(0xFF, 1);
			write_column(0xFF, 2);
			write_column(0xFF, 3);
			write_column(0xFF, 4);
			write_column(0xFF, 5);
			write_column(0xFF, 6);
			write_column(0xFF, 7);
			if(any_pressed())
			{
				set_state(GAME);
				init_game();
			}
		}
	}
}
/////////////////////////////<-----------------/MAIN//////////////////////////////////////////////////////
