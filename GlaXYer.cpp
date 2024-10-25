#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<windows.h>
#include<conio.h>
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

#define CORNER_BL 200
#define CORNER_TL 201
#define LINE_H    205
#define LINE_V    186
#define CORNER_TR 187
#define CORNER_BR 188
#define MAX_HEIGHT 20
#define MAX_WIDTH 40

void setCursorPosition(int y, int x){
	printf("\033[%d;%dH", y, x);
}

struct Position{ // coordinate
	int x;
	int y;
};

struct Field{
	Position p; // field position from root(0,0), can be negative
	Field *left;
	Field *right;
	Field *top;
	Field *bottom;
} *root;

struct User{
	Position p;
	int baseLevel;
	int iceHeartCount;
	Field *currentField;;
	char name[1000];
};

Field *createField(Position p){
	Field *newField = (Field*)malloc(sizeof(Field));
	newField->left = newField->right = newField->top = newField->bottom = NULL;
	newField->p = p;
	return newField;
}


Field *fieldLinkedHead;



void printField(User *user) {
    setCursorPosition(1, 1);

    printf(CYAN "%c", CORNER_TL);
    for (int i = 0; i < MAX_WIDTH; i++) {
        printf("%c", LINE_H); 
    }
    printf("%c\n" RESET, CORNER_TR);
    
    for (int y = 0; y < MAX_HEIGHT; y++) {
		setCursorPosition(2+y, 1); // Adjust cursor for each row
        printf(CYAN "%c" RESET, LINE_V); // Left vertical line
        
        for (int x = 0; x < MAX_WIDTH; x++) {
            if (x == user->p.x && y == user->p.y) {
                printf("P"); // Player's position
            } else {
                printf(" ");
            }
        }

        printf(CYAN "%c\n" RESET, LINE_V);
    }

    // Printing the bottom border
    setCursorPosition(22, 1);
    printf(CYAN "%c", CORNER_BL); // Bottom-left corner
    for (int i = 0; i < MAX_WIDTH; i++) {
        printf("%c", LINE_H); // Horizontal lines
    }
    printf("%c\n" RESET, CORNER_BR); // Bottom-right corner
    setCursorPosition(1,1);
}

Field *findField(Position p) {
    // Start from root
    Field *current = root;
    
    // Try to find the field with given coordinates
    while (current != NULL) {
        if (current->p.x == p.x && current->p.y == p.y) {
            return current;
        }
        
        // Navigate based on relative position
        if (current->p.x < p.x && current->right != NULL) {
            current = current->right;
        } else if (current->p.x > p.x && current->left != NULL) {
            current = current->left;
        } else if (current->p.y < p.y && current->top != NULL) {
            current = current->top;
        } else if (current->p.y > p.y && current->bottom != NULL) {
            current = current->bottom;
        } else {
            // Can't find a path to the desired position
            return NULL;
        }
    }
    return NULL;
}

Field *connectField(char direction, Field *sourceField, Field *newField) {
    if (!sourceField || !newField) return NULL;
    
    switch(direction) {
        case 'L':
            sourceField->left = newField;
            newField->right = sourceField;
            break;
        case 'R':
            sourceField->right = newField;
            newField->left = sourceField;
            break;
        case 'T':
            sourceField->top = newField;
            newField->bottom = sourceField;
            break;
        case 'B':
            sourceField->bottom = newField;
            newField->top = sourceField;
            break;
    }
    return newField;
}

void initializeMap() {
    // Create root field at (0,0)
    Position rootPos = {0, 0};
    root = createField(rootPos);
}

Field *getOrCreateField(Position p) {
    Field *existingField = findField(p);
    if (existingField) {
        return existingField;
    }
    
    return createField(p);
}

void handleFieldTransition(User *user, char direction) {
    Position newFieldPos = user->currentField->p;
    Position newUserPos = user->p;
    
    switch(direction) {
        case 'L':
            newFieldPos.x--;
            newUserPos.x = MAX_WIDTH - 1;
            break;
        case 'R':
            newFieldPos.x++;
            newUserPos.x = 0;
            break;
        case 'B':
            newFieldPos.y++;
            newUserPos.y = 0;
            break;
        case 'T':
            newFieldPos.y--;
            newUserPos.y = MAX_HEIGHT - 1;
            break;
    }
    
    // Get or create the new field
    Field *newField = getOrCreateField(newFieldPos);
    
    // Connect the new field with the current field
    connectField(direction, user->currentField, newField);
    
    // Update user's position and current field
    user->p = newUserPos;
    user->currentField = newField;
    
    // Check and connect with any existing adjacent fields
    Position adjacentPos;
    Field *adjacentField;
    
    // Check left
    adjacentPos = newFieldPos;
    adjacentPos.x--;
    adjacentField = findField(adjacentPos);
    if (adjacentField) connectField('L', newField, adjacentField);
    
    // Check right
    adjacentPos = newFieldPos;
    adjacentPos.x++;
    adjacentField = findField(adjacentPos);
    if (adjacentField) connectField('R', newField, adjacentField);
    
    // Check top
    adjacentPos = newFieldPos;
    adjacentPos.y++;
    adjacentField = findField(adjacentPos);
    if (adjacentField) connectField('B', newField, adjacentField);
    
    // Check bottom
    adjacentPos = newFieldPos;
    adjacentPos.y--;
    adjacentField = findField(adjacentPos);
    if (adjacentField) connectField('T', newField, adjacentField);
}

void moveUser(User *user, char input) {
    int newX = user->p.x;
    int newY = user->p.y;
    
    switch (input) {
        case 'w': case 'W':
            if (newY > 0) {
                user->p.y--;

            } else {
                handleFieldTransition(user, 'T');

            }
            break;
        case 's': case 'S':
            if (newY < MAX_HEIGHT-1) {
                user->p.y++;
            } else {
                handleFieldTransition(user, 'B');
            }
            break;
        case 'a': case 'A':
            if (newX > 0) {
                user->p.x--;
            } else {
                handleFieldTransition(user, 'L');
            }
            break;
        case 'd': case 'D':
            if (newX < MAX_WIDTH-1) {
                user->p.x++;
            } else {
                handleFieldTransition(user, 'R');
            }
            break;
    }
}

void moveO(User *user){
   char input;

    // Game loop
    while (1) {
    	system("cls");
        printField(user);       // Print the map and player's position

        input = getch();        // Get the player's input
        moveUser(user, input);  // Update the player's position based on input

        if (input == 'q' || input == 'Q') break; // Quit if 'q' is pressed
    }
}

int main() {
    HWND consoleWindow = GetConsoleWindow();
    ShowWindow(consoleWindow, SW_MAXIMIZE);
    
    initializeMap();
    
    User *user = (User*)malloc(sizeof(User));
    user->p.x = (MAX_WIDTH-1)/2;
    user->p.y = (MAX_HEIGHT-1)/2;
    user->baseLevel = 1;
    user->iceHeartCount = 0;
    user->currentField = root;  // Start at root field
    
    moveO(user);
    
    return 0;
}
