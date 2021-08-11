
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef enum {MOV,ADD,SUB,AND,OR,NOT,OUT}opcodes;
typedef enum {WR,XR,YR,ZR}registers;
typedef enum {OPCODE,REG,IMM,COMMA,ERROR,EndF,CHNGL} tokentype;
typedef enum {start,indig,inkey,retimm,retkey,retcomm,end}states ;


char *buffer;   //this is to find the token string
size_t lenbuffer = 5;   //Size of buffer
unsigned int b=0;   //buffer index

char *line;     //current line in the source file
size_t len=100;   //buffer size for the source file

tokentype token; //This will be used to construct the binary instruction

unsigned int lineno=0;   //Line number in source file
unsigned int current=0;  //current character in the line contained in line variable


int locop = -1;    //Index of the opcode in the optable lookup table
int locreg = -1;    //Index of the register in the register lookup table

int8_t immediate=0;     //immediate value

char getCurrentChar()    //Function to extract current character from the line
{
    char c = line[current];
    current++;
    return c;
}

void insertchar(char ch){    //Function to insert character in the buffer
    buffer[b] = ch;
    b++;
}
void initbuffer()       //Function to initialize the buffer before a token reading operation
{
    buffer[0]='\0';
    buffer[1]='\0';
    buffer[2]='\0';
    buffer[3]='\0';
    buffer[4]='\0';
    b=0;
}


struct opcode_table        //Opcode lookup table Declaration
{
    const char *mnemonic;
    opcodes opcode;
};

struct register_table    //Register lookup table declaration
{
    const char *reg_name;
    registers reg_code;
};



struct opcode_table optable[7]={
    {"mov",MOV},
    {"add",ADD},
    {"sub",SUB},
    {"and",AND},
    {"or",OR},
    {"not",NOT},
    {"out",OUT}
};

struct register_table regtable[4]={
    {"wr",WR},
    {"xr",XR},
    {"yr",YR},
    {"zr",ZR}
};

typedef union instruction{
    struct ifields{
        int8_t fmtbit:1;
        int8_t src_reg:2;
        int8_t dest_reg:2;
        int8_t opcode:3;
    }field;
    int8_t value;
}finalinstruction;


int IsValidOpcode(char *str)
{
    int i;
    for(i=0;i<7;i++)
    {
        if(strcmp(str,optable[i].mnemonic) == 0) break;
    }
    if(i==7) {i=-1;}
    return i;
}

int IsValidRegister(char *str)
{
    int i;
    for(i=0;i<4;i++)
    {
        if(strcmp(str,regtable[i].reg_name) == 0) break;
    }
    if(i==4) {i=-1;}
    return i;
}

void GetToken()
{
    states state = start;
    initbuffer();
    immediate=0;
    char ch = '\0';
    while(state != end)
    {
        switch(state)
        {
            case start:
                ch = getCurrentChar();
                while(ch == ' ' || ch == '\t') ch=getCurrentChar();
                if(ch == '\n'){
                    token = CHNGL;
                    state=end;
                    break;
                }
                if(ch == '\0'){
                    token = EndF;
                    state=end;
                    break;
                }
                if(isdigit(ch)) state = indig;
                else if(isalpha(ch)) state = inkey;
                else if(ch == ',') state = retcomm;
                else {
                    printf("Line %i: Invalid character detected!\n",lineno);
                    token = ERROR;
                }
                break;
            case indig:
                while(isdigit(ch)){
                    immediate = immediate * 10 + (ch - 0x30);
                    ch=getCurrentChar();
                }
                state = retimm;
                break;
            case inkey:
                while(isalpha(ch))
                {
                    insertchar(ch);
                    ch=getCurrentChar();
                }
                state = retkey;
                break;
            case retimm:
                token = IMM;
                state=end;
                break;
            case retkey:
                current--;
                if((locop = IsValidOpcode(buffer))>=0){
                    token = OPCODE;
                }
                else if((locreg = IsValidRegister(buffer))>=0){
                    token = REG;
                }
                else{
                    printf("Line %i: Error! Invalid opcode or register!\n",lineno);
                    token = ERROR;
                }
                state = end;
                break;
            case retcomm:
                token=COMMA;
                state=end;
                break;
            case end:
                break;
        }
        
    }
        
}

int main(int argc,char *argv[])
{
    FILE *source;
    finalinstruction instr;
    FILE *destf;
    line = (char*)malloc(len);
    buffer=(char*)malloc(lenbuffer);
    
    source = fopen(argv[0],"r");
    if(source == NULL){
        printf("Error! Source file not found!\n");
        exit(1);
    }
    
    destf=fopen(argv[1],"w");
    printf("Assembly started!\n");
    fprintf(destf,"v2.0 raw\n");
    while(!feof(source))
    {
        lineno++;
        current=0;
        instr.value=0;
        getline(&line,&len,source);
        GetToken();
        if(token == EndF) break;
        if(token == CHNGL) continue;
        if(token == OPCODE)
        {
            instr.field.opcode = optable[locop].opcode;
            if(optable[locop].opcode == OUT)
            {
                GetToken();
                if(token == REG)
                {
                    instr.field.dest_reg = regtable[locreg].reg_code;
                    instr.field.src_reg = 0;
                    instr.field.fmtbit=0;
                    fprintf(destf,"%hhu\n",instr.value);
                    continue;
                }
                else{
                    printf("Line %i: Error! Register operand not found!\n",lineno);
                    fprintf(destf,"Error\n");
                    break;
                }
            }
        }
        else{
            printf("Line %i: Error! Invalid opcode found!\n",lineno);
            fprintf(destf,"Error\n");
            break;
        }
        
        GetToken();
        if(token == EndF) break;
        if(token == CHNGL) continue;
        if(token == REG)
        {
            instr.field.dest_reg = regtable[locreg].reg_code;
        }
        else{
            printf("Line %i: Error! Destination operand must be a register!\n",lineno);
            fprintf(destf,"Error\n");
            break;
        }
        
        GetToken();
        if(token == EndF) break;
        if(token == CHNGL) continue;
        if(token != COMMA){
            printf("Line %i: Missing ',' \n",lineno);
            fprintf(destf,"Error\n");
            break;
        }
        
        GetToken();
        if(token == EndF) break;
        if(token == CHNGL) continue;
        if(token == REG)
        {
            instr.field.src_reg = regtable[locreg].reg_code;
            instr.field.fmtbit = 0;
            fprintf(destf,"%hhu\n",instr.value);
        }
        else if(token == IMM)
        {
            instr.field.src_reg = 0;
            instr.field.fmtbit = 1;
            fprintf(destf, "%hhu\n",instr.value);
            fprintf(destf, "%hhu\n",immediate);
        }
        else{
            printf("Line %i: Error! Either register or 8-bit immediate value expected!\n",lineno);
            fprintf(destf,"Error\n");
            break;
        }
    }
    
    free(line);   //Deallocate line buffer
    free(buffer);   //Deallocate token buffer
    fclose(destf);
    fclose(source);
    return 0;
}
