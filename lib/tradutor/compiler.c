#include <stdio.h>
#include <string.h>

#include "base.h"  // Aqui estão definidos o buffer e o read_line()

int function_initialization();
void process_local_variables();
void process_instructions();
void process_attribuition();
void process_vector_getter();

//Essas structs servem para salvarmos as informacoes necessarias e uteis das nossas variaveis locais para facilitar o uso do registrador da pilha correspondentes a uma variavel.
typedef struct stack_info
{
	int offset;
	unsigned int size;
} stack_info;

//temos no maximo 5 variaveis locais.
stack_info stack[5];

void compile_function()
{
	// Registradores:
	// Primeiro parâmetro: %di
	// Segundo parâmetro: %si
	// Terceiro parâmetro: %dx
	int parameters = function_initialization();
	
	// Inicializar a pilha

	printf("    pushq %%rbp\n");
	printf("    movq %%rsp, %%rbp\n");

	// Processar as variáveis locais

	process_local_variables();
	
	// Processar as instruções

	process_instructions();

	// Desfazer a pilha

	printf("    leave\n");
	printf("    ret\n\n");
}

int function_initialization()
{
	int function_number = 0;
	char parameter_types[3];

	int matches = sscanf(buffer, "function f%d p%c1 p%c2 p%c3", &function_number,
						 &parameter_types[0], &parameter_types[1], &parameter_types[2]);

	// Sabemos que pelo menos o primeiro %d foi lido com sucesso.
	// Para descobrir a quantidade de parâmetros. Devemos olhar quantos matches foram feitos.

	printf(".globl f%d\n", function_number);
	printf("f%d:\n", function_number);

	return matches - 1;
}

void process_local_variables()
{
	// Buffer atual: "function fN {pX1 {pX2 {pX3}}}"

	read_line();  	// Buffer atual: "def"
	// Agora iremos ler as próximas linhas até encontrar "enddef"
	// E alocar o espaço necessário na pilha.

	int required_bytes = 0;
	int index;
		
	do
	{
		read_line();
		if(strncmp(buffer, "var", 3) == 0)
		{
			sscanf(buffer, "var vi%d", &index);
			
			required_bytes = 4;

			stack[index-1].offset = required_bytes * (-1); 
			continue;
		}
		if(strncmp(buffer, "vet", 3) == 0)
		{
			int vector_size = 0;
			sscanf(buffer, "vet va%d size ci%d", &index, &vector_size);

			required_bytes += 4 * vector_size;

			stack[index-1].size = vector_size;
			stack[index-1].offset = required_bytes * (-1);
		}


	} while(strncmp(buffer, "enddef", 6) != 0);

	// Agora que sabemos quantos bytes precisamos, hora de alocar a pilha.
	// Lembrando que a pilha deve sempre ser alocada em múltiplos de 16

	if(required_bytes == 0)
		return;

	int stack_size = 0;
	while(stack_size < required_bytes)
		stack_size += 16;

	printf("    subq $%d, %%rsp\n\n", stack_size);
}

void process_instructions()
{
	// Buffer atual: "enddef"

	read_line();	// Buffer atual: ??? ("end" ou instrução)

	while(strncmp(buffer, "end", 3) != 0)
	{
		if(strncmp(buffer, "get", 3) == 0) process_vector_getter();
		//if(strncmp(buffer, "set", 2) == 0) process_vector_setter();
		read_line();
	}
}

void process_vector_getter()
{
	// Buffer atual: "get CaN index ciN to CiN"

	char vec_type;	// variável local ou parâmetro
	int vec_index;	// índice do vetor

	int vec_offset;	// A posição a ser acessada

	char target_type;	// variável local ou parâmetro
	int target_index;	// índice do target

	sscanf(buffer, "get %ca%d index ci%d to %ci%d", 
		&vec_type, &vec_index, &vec_offset,
		&target_type, &target_index);

	char register_pointer[4];

	if(vec_type == 'v')
	{
		strcpy(register_pointer, "rbp");
		vec_offset = stack[vec_offset - 1].offset;
	}
	else	// vec_type = "p"
	{
		switch (vec_index)
		{
			case 1:
				strcpy(register_pointer, "edi");
				break;
			case 2:
				strcpy(register_pointer, "esi");
				break;
			case 3:
				strcpy(register_pointer, "edx");
				break;
		}

		vec_offset *= 4;
	}

	printf("    movl %d(%%%s), ", vec_offset, register_pointer);

	// TODO: Calcular a posição da variável destino
	if(target_type == 'p')
	{
		switch (target_index)
		{
			case 1:
				printf("%%edi");
				break;
			case 2:
				printf("%%esi");
				break;
			case 3:
				printf("%%edx");
				break;
		}
	}
	else	// target_type == 'v'
	{
		printf("-%d(%%rbp)", stack[target_index].offset);
	}


	// TODO: Atualizar o valor da variável destino
}