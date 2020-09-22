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
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <stdio.h>
////////////////////////////////------------>CONSTANTES////////////////////////////////////////////////////
struct Position
{
	int8_t x;
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
struct Snake
{
	// Su cuerpo puede ocupar todo el mundo.
	struct Position body[35]; 
	struct Position head, tail;
	// El estomago mantiene la puntuación.
	uint8_t belly;
	// El largo de la serpiente. 
	uint8_t length;
}snake;
struct World
{
	// Multiplicación de la matriz, en este caso es 5*7 = 35.
	enum Cell grid[35];
	// Altura de 5.
	uint8_t height;
	// Ancho de 7.
	uint8_t width;
	// El mundo cuenta con un estado, en GAME o MENU.
	enum State state;
}world;
volatile uint8_t button_g = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////------------->FUNCIONES////////////////////////////////////////////////////
void game_over()
{
	PORTB = 0x00;
	PORTC = 0xFF;
}
void init_interrupts()
{
	EICRA |= (1 << ISC11)|(1 << ISC10)|(1 << ISC11)|(1 << ISC10);
	// Es necesario habilitar los pines, para que el microcontrolador
	// active la función.
	EIMSK |= (1 << INT1)|(1 << INT0);
	sei();
	//Siempre activan en cambio de estado
	PCICR |= (1<<PCIE0)|(1<<PCIE2);
	PCMSK2 |= (1<<PCINT0);
	PCMSK0 |= (1<<PCINT7);
}
const struct Snake *get_snake()
{
	return &snake;
}
// Se ignoran direcciones sin sentido para que la serpiente no se coma a si misma. 
void handle_keypress(const struct Position *_direction)
{
	// Se ignoran las direcciones sin sentido
	if(_direction->x == _direction->y)
	{
		return;
	}
	if(_direction->x != 0 && _direction->y != 0)
	{
		return;
	}
	// Se ignoran las direcciones conflictivas, como hacer la serpiente ir arriba cuando debe ir abajo
	// O ir a la derecha cuando debe ir a la izquierda.
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
void set_cell(const struct Position *position, enum Cell value)
{
	// Coloca un valor enumerado de la instancia value de Cell en
	// una posición concreta. 
	world.grid[position->y * world.width + position->x] = value;
}
struct Position direction_to_position(enum Cell direction)
{
	struct Position position = {0, 0};
	switch (direction)
	{
		case SNAKE_UP:
		{
			position.y--;
			break;
		}
		case SNAKE_DOWN:
		{
			position.y++;
			break;
		}
		case SNAKE_LEFT:
		{
			position.x--;
			break;
		}
		case SNAKE_RIGHT:
		{
			position.x++;
			break;
		}
		default:
		{
			break;	
		}
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
enum Cell cell_at(const struct Position *position)
{
	// Regresa el punto de una posición dentro del mundo "Snake".
	// Nos dice la dirección, comida o bonus.
	return world.grid[position->y * world.width + position->x];
}
void place_food()
{
	// To do : El programa se puede congelar si la serpiente crece demasiado.
	// Mientras no este vacia la celda, se puede colocar una Comida, al ser 
	// ser comida se entra en un BONUS.
	do
	{
		// Coordenadas de la estructura posición, denominada comida.
		food.x = rand() % world.width; 
		food.y = rand() % world.height;
	}while(cell_at(&food) != EMPTY);
	set_cell(&food, FOOD);
}
void init_game()
{
	// Se usa un valor para la semilla, garantizando una secuencia diferente en cada iteración.
	srand(18979);
	direction.x = 1;
	direction.y = 0;
	world.height = 5;
	world.width = 7;
	// Puntuación.
	snake.belly = 0;
	// Cuerpo de la serpiente.
	snake.length = 0;
	// Se inician vacio el mundo.
	uint8_t c;
	for (c = 0; c < world.width * world.height; c++)
	{
		world.grid[c] = EMPTY;
	}
	// Se posiciona en medio.
	struct Position pos = {world.width / 2, world.height / 2};
	set_cell(&pos, SNAKE_LEFT);
	snake.head = pos;
	pos = (struct Position){world.width / 2 + 1, world.height / 2};
	set_cell(&pos, SNAKE_LEFT);
	pos = (struct Position){world.width / 2 + 2, world.height / 2};
	set_cell(&pos, SNAKE_LEFT);
	snake.tail = pos;
	// Se coloca la comida.
	place_food();
	// Se ignoran los movimientos donde la cabeza y la cola se tocan.
	handle_keypress(&direction);
}
enum State get_state(void)
{
	// Getter del estado del juego.
	return world.state;
}
void set_state(enum State state)
{
	// Setter del estado del juego.
	world.state = state;
}
void handle_wrapping(struct Position *position)
{
	// Detección de los bordes con los limites de la estructura mundo.
	if (position->x < 0)
	{
		set_state(MENU);
	}
	else if (position->x >= world.width)
	{
		set_state(MENU);
	}
	if (position->y < 0)
	{
		set_state(MENU);
	}
	else if (position->y >= world.height)
	{
		set_state(MENU);
	}
}
void move_head(const struct Position *head)
{
	// Si la cabeza y la cola estan en las mismas coordenadas, se reinicia el juego.
	if (head->x == snake.tail.x && head->y == snake.tail.y)
	{
		// Se crea una nueva cola, con las coordenadas de la última posición ++ o --.
		struct Position new_tail = direction_to_position(cell_at(&snake.tail));
		// Se renuevan los valores de la posición, sumando la anterior con la recien creada (en x y y).
		struct Position tail = 
		{
			.x = snake.tail.x + new_tail.x, .y = snake.tail.y + new_tail.y
		};
		// Checa si se encuentra dentro de los bordes. 
		handle_wrapping(&tail);
		// La cabeza es igual al valor donde estaba la cola. 
		snake.head = snake.tail;
		return;
	}
	// Se pasa la direccion en donde se moverá.
	enum Cell head_direction = position_to_direction(&direction);
	// Se renueva la posición a la que se apuntaba.
	snake.head = *head;
	// Se coloca el valor de la nueva cabeza y la dirección a la que se moverá.
	set_cell(&snake.head, head_direction);
}
void move_tail(void)
{
	// Se extrae la posición.
	struct Position tail = snake.tail;
	// Se extrae la dirección. 
	enum Cell tail_direction = cell_at(&snake.tail);
	// Se realiza el movimiento. 
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
			break;
	}
	// Se verifican los limites.
	handle_wrapping(&tail);
	// Caso default. 
	if (snake.head.x == snake.tail.x && snake.head.y == snake.tail.y)
	{
		set_cell(&snake.tail, tail_direction);
	}
	else
	{
		// Apaga la cola para encenderce en la otra dirección. 
		set_cell(&snake.tail, EMPTY);
	}
	// Se renueva la posición de la cola. 
	snake.tail = tail;
}
const struct World *get_world(void)
{
	// Se regresa mudno referenciado, ya que es un struct y se quieren conservar los valores intanciados en sus
	// atributos.
	return &world;
}
void eat_food(const struct Position *head, enum Cell food_type)
{
	// Mueves la cabeza.
	move_head(head);
	// Se aumenta la puntuación.
	if(food_type == FOOD)
	{
		snake.belly += 1;
	}
	else
	{
		snake.belly += 5;
	}
	// Se coloca la comida.
	place_food();
	// Se hace la serpiente más grande.
	snake.length++;
}
// Función de controlar el tiempo y renderizar el mundo en la matriz. 
void tick(void)
{
	// Pasa la dirección, para renderizar en la siguiente dirección.
	set_cell(&snake.head, position_to_direction(&direction));
	// Se genera la nueva dirección.
	struct Position head = 
	{
		.x = direction.x + snake.head.x,
		.y = direction.y + snake.head.y
	};
	// Se verifica que este dentro de los limites.
	handle_wrapping(&head);
	// Se extrae el contenido de la celda en contacto con la cabeza.
	enum Cell cell = cell_at(&head);
	switch (cell)
	{
		case FOOD:
			eat_food(&head, cell);
			break;
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
ISR(PCINT2_vect)
{
	button_g = 4;
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
	PORTB = ~(1 << column);
	PORTC = data;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////------------------>RENDER GAME///////////////////////////////////////////////
void render(int repetitions)
{
	while(repetitions--)
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
				{
					data |= 1 << row;	
				}
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
	uint8_t tick_interval = 30;
	for(;;)
	{
		if(get_state() == GAME)
		{
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
			render(5);
			_delay_ms(8);
			if(tick_interval-- == 0)
			{
				tick_interval = 30;
				tick();
			}
		}
		if(get_state() == MENU)
		{
			game_over();
			_delay_ms(1000);
			set_state(GAME);
			init_game();
		}
	}
}
/////////////////////////////<-----------------/MAIN//////////////////////////////////////////////////////
