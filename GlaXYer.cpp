#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<windows.h>
#include<conio.h>
#include<math.h>
#define CYAN "\x1b[36m"
#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"
#define HIGHLIGHT_COLOR "\033[30;47m" 
#define ABSOLUTE_VALUE(x) ((x) < 0 ? -(x) : (x))

#define CORNER_BL 200
#define CORNER_TL 201
#define LINE_H    205
#define LINE_V    186
#define CORNER_TR 187
#define CORNER_BR 188
#define T_SPLIT   203
#define B_SPLIT   202
#define L_SPLIT   204
#define R_SPLIT   185
#define CROSS     206
#define MAX_HEIGHT 20
#define MAX_WIDTH 40


void flushBuffer(){
	while(kbhit()){
		getch();
	}
}

void setCursorPosition(int y, int x){
	printf("\033[%d;%dH", y, x);
}

struct Position{ // coordinate
	int x;
	int y;
};

struct Card{
	// type 1 = attack, type 2 deffense, type 3 buff, type 4 heal
	int type;
	// attack : nyerang dengan base attack player, 15% critical untuk 300% damage
	// deffense : reduce damage selanjutnya sebanyak 50%
	// buff : ningkatin attack 2x, bisa di stack jadi 2x 4x 8x dst, kalau setelah buff trus attack, reset ke 1
	// heal : akan nambah darah sebanyak 25% dari max HP user, cuma tersedia untuk musuh base level 3 atau
	// user base level apapun
};

struct Dungeon {
	Position p;
	int baseLevel;
	int enemyHP;
	int enemyBaseAttack;
	int enemyCardCount;
	Card enemyCard[8];
};



struct Field{
	Position p; // field position from root(0,0), can be negative
	Field *left;
	Field *right;
	Field *top;
	Field *bottom;
	Dungeon dungeon;
} *root;

struct User{
	Position p;
	int baseLevel;
	int iceHeartCount;
	Field *currentField;
	char name[1000];
	int baseHp;
	int baseAttack;
	int cardCount;
};

struct FieldVault{
	Field *field;
	FieldVault *next;
} *vHead;

struct Bookmark {
	char notes[1000];
	Field *field;
} *hashTable[1000] = {NULL};


unsigned int hashFunction(const char *str) {
    unsigned int hash = 0;
    unsigned int prime = 31;
    while (*str) {
        hash = (hash * prime) + (*str++); // Calculate hash value
    }
    return hash % 1000; 
}

Bookmark *createBookmark(const char *notes, Field *field){
	Bookmark *newBookmark = (Bookmark *)malloc(sizeof(Bookmark));
    strcpy(newBookmark->notes, notes);
    newBookmark->field = field;
    return newBookmark;
}

void insertBookmark(const char *notes, Field *field) {
    // Create a new bookmark
    Bookmark *newBookmark = createBookmark(notes, field);

    // Generate hash key
    unsigned int hashIndex = hashFunction(notes);

    // Handle collision using linear probing
    unsigned int originalIndex = hashIndex;
    while (hashTable[hashIndex] != NULL) {
        if (strcmp(hashTable[hashIndex]->notes, notes) == 0) {
            free(hashTable[hashIndex]);
            hashTable[hashIndex] = newBookmark;
            return;
        }
        hashIndex = (hashIndex + 1) % 1000;
        if (hashIndex == originalIndex) {
            free(newBookmark);
            return;
        }
    }

    hashTable[hashIndex] = newBookmark;
}

int deleteBookmark(const char *notes) {
    int hashKey = hashFunction(notes);
    int originalHashKey = hashKey;

    while (hashTable[hashKey] != NULL) {
        if (strcmp(hashTable[hashKey]->notes, notes) == 0) {
            free(hashTable[hashKey]);
            hashTable[hashKey] = NULL;

            int nextKey = (hashKey + 1) % 1000;
            while (hashTable[nextKey] != NULL) {
                Bookmark *temp = hashTable[nextKey];
                hashTable[nextKey] = NULL;

                int newHashKey = hashFunction(temp->notes);
                while (hashTable[newHashKey] != NULL) {
                    newHashKey = (newHashKey + 1) % 1000;
                }
                hashTable[newHashKey] = temp;

                nextKey = (nextKey + 1) % 1000;
            }

            return 1;
        }

        hashKey = (hashKey + 1) % 1000;

        if (hashKey == originalHashKey) break;
    }

    return 0; 
}



int getDiff(int pos1, int pos2){
	// usahain pos1 untuk playerPos, pos2 untuk currPointerPos
	// misalnya posisi yang mau di print di x = 1, playerPos di x = 5
	// maka 1-5 = -4
	// sebelum player pos itu maksimal cm ada 5 maka -4 '+ 6' untuk dapetin x di 2
	return pos2 - pos1;
}

FieldVault *createVault(Field *field){
	FieldVault *fv = (FieldVault*)malloc(sizeof(FieldVault));
	fv->next = NULL;
	fv->field = field;
	return fv;
}

Field *createField(Position p){
	Field *newField = (Field*)malloc(sizeof(Field));
	newField->left = newField->right = newField->top = newField->bottom = NULL;
	newField->p = p;
	// di base map gaada dungeon
	if(p.x != 0 || p.y != 0){
		newField->dungeon.p.x = (rand() % 36) + 2;
		newField->dungeon.p.y = (rand() % 16) + 2;
//		int randNum = (rand() % 100) < 33 ? 1 : (rand() % 100) < 75 ? 2 : 3;
		newField->dungeon.baseLevel = 1;
		newField->dungeon.enemyBaseAttack = 100;
		newField->dungeon.enemyCardCount = 5;
		newField->dungeon.enemyHP = 500;
		for(int i = 0; i < 5; i++){
			// random angka 1 sampe 3
			// 1 attack, 2 deffense, 3 buff
			newField->dungeon.enemyCard[i].type = (rand() % 100) < 50 ? 1 : (rand() % 100) < 30 ? 2 : 3;
		}
	}
	return newField;
}


void insertNewVault(Field *field){
	if(vHead == NULL){
		vHead = createVault(field);
		return;
	}
	FieldVault *nv = createVault(field);
	nv->next = vHead;
	vHead = nv;
}

void printBlankLine(){
	printf("                                                                            ");
}
 
void resetScreen(){
	for(int i = 1; i <= 45; i++){
		setCursorPosition(i, 1);
		printBlankLine();
		printf("                            ");
	}
} 

void printMapBox(int ySize, int xSize){
	// ySize  untuk define size tinggi kotak kosong (tidak termasuk frame)
	// xSize untuk define size lebar kotak kosong (tidak termasuk frame)
	// hilangin resetScreen trus ntar taro di bagian dimana itu perlu aja  (disetiap scene)
	resetScreen();
	setCursorPosition(1,1);
	printf(CYAN "%c", CORNER_TL);
	for (int i = 0; i < xSize; i++) {
        printf("%c", LINE_H); 
    }
    printf("%c\n" RESET, CORNER_TR);
    
    for (int y = 0; y < ySize; y++) {
		setCursorPosition(2+y, 1); 
        printf(CYAN "%c" RESET, LINE_V); 
        
        for (int x = 0; x < xSize; x++) {
			printf(" ");
        }

        printf(CYAN "%c\n" RESET, LINE_V);
    }
    setCursorPosition(2+ySize, 1);
    printf(CYAN "%c", CORNER_BL); 
    for (int i = 0; i < xSize; i++) {
        printf("%c", LINE_H); 
    }
    printf("%c\n" RESET, CORNER_BR);
    setCursorPosition(1,1);

}

void printUserPosition(User *user){
	setCursorPosition(user->p.y + 2, user->p.x + 2);
    printf("P");
}

void printField(User *user) {
	
	// nvm
	printMapBox(MAX_HEIGHT, MAX_WIDTH);
	setCursorPosition(2 + (user->p.y), 2 + (user->p.x));
	printf("P");
	if(user->currentField->p.x != 0 || user->currentField->p.y != 0){
		Position dungeonPosition = user->currentField->dungeon.p;
		setCursorPosition(1 + dungeonPosition.y, 1 + dungeonPosition.x);
		// 220 atas, 221 kiri, 222 kanan, 207 pintu
		printf(user->currentField->dungeon.baseLevel < 2 ? GREEN :	
		(user->currentField->dungeon.baseLevel < 3) ?
		YELLOW : (user->currentField->dungeon.baseLevel < 4) ? RED : CYAN);
		printf("%c%c%c" RESET, 220, 220, 220);
		
		setCursorPosition(2 + dungeonPosition.y, 1 + dungeonPosition.x);
		printf(user->currentField->dungeon.baseLevel < 2 ? GREEN :
		(user->currentField->dungeon.baseLevel < 3) ?
		YELLOW : (user->currentField->dungeon.baseLevel < 4) ? RED : CYAN);
		printf("%c%c%c" RESET, 221, 207, 222);
	}
	else {
		// 219 block
		// base 5x7
		setCursorPosition((MAX_HEIGHT/2) - 1,(MAX_WIDTH/2) - 3);
		printf("%c%c%c%c%c%c%c", CORNER_TL,LINE_H, LINE_H, LINE_H, LINE_H, LINE_H,CORNER_TR);
		// 176 barikade
		setCursorPosition((MAX_HEIGHT/2)   ,(MAX_WIDTH/2) - 3);
		printf("%c%c%c%c%c%c%c", LINE_V,176, 176, 176, 176, 176,LINE_V);
		// 143 tower
		setCursorPosition((MAX_HEIGHT/2) + 1,(MAX_WIDTH/2) - 3);
		printf("%c%c%c%c%c%c%c", LINE_V, 176, 176,143,176, 176, LINE_V);
		
		setCursorPosition((MAX_HEIGHT/2) + 2,(MAX_WIDTH/2) - 3);
		printf("%c%c%c%c%c%c%c", LINE_V,176,176,176,176,176,LINE_V);
		
		setCursorPosition((MAX_HEIGHT/2) + 3,(MAX_WIDTH/2) - 3);
		printf("%c%c%c%c%c%c%c", CORNER_BL, LINE_H, LINE_H, LINE_H, LINE_H, LINE_H, CORNER_BR);
	}
	
	setCursorPosition(5, 44);
    printf("Base Level      : %d\n", user->baseLevel);
    setCursorPosition(6, 44);
    printf("User Max HP     : %d", user->baseHp);
    setCursorPosition(7, 44);
    printf("User Attack     : %d",  user->baseAttack);
    setCursorPosition(8, 44);
    printf("Ice Heart Owned : %d\n", user->iceHeartCount);
    setCursorPosition(9, 44);
    printf("Card Slot       : %d\n", user->cardCount);
    setCursorPosition(10, 44);
    printf("Username        : %s\n", user->name);
}

void printFieldSummary(User *user){
	// untuk print map summary (yang udah dilewati sama user) dengan radius 5 disekitar player
	if (!vHead) return;
	
	FieldVault *current = vHead;
	Position userMapPos = user->currentField->p;
	resetScreen();
	printMapBox(11,23);
	while(current) {
		Position currPos = current->field->p;
		// cek apakah masing masing field ada diarea 5 dari user
		int checkOne = currPos.x >= userMapPos.x - 5 && currPos.x <= userMapPos.x + 5;
		int checkTwo = currPos.y >= userMapPos.y - 5 && currPos.y <= userMapPos.y + 5;
		if (checkOne && checkTwo){
			// rumus = currentPos - playerPos + 6
			int difX = getDiff(userMapPos.x, currPos.x);
			int difY = getDiff(userMapPos.y, currPos.y);
			setCursorPosition(difY + 7, (difX*2) + 13);
			// priority -> user ada di root map, kalau ga rootmapnya di print O aja
			printf((currPos.x == 0 && currPos.y == 0 ) ? 
			(difX ==  0 && difY == 0 ? CYAN "O" RESET : "O") 
			: 
			(difX ==  0 && difY == 0 ? CYAN "P" RESET : "X"));
		}
		current = current->next;
	}
	getch();
	
	// Balik ke map jalan jalan
	printMapBox(MAX_HEIGHT, MAX_WIDTH);
	printField(user);
}

Field *findField(Position p) {
    if(vHead->field->p.x == p.x && vHead->field->p.y == p.y){
    	return vHead->field;
	}
	FieldVault *current = vHead;
	while (current != NULL){
		if(current->field->p.x == p.x && current->field->p.y == p.y){
			return current->field;
		}
		current = current->next;
	}
	return NULL;
}

Field *connectField(char direction, Field *sourceField, Field *newField) {
	// cuma untuk menghubungkan antara 2 field doang, sistemnya hubungin satu persatu
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
    insertNewVault(root);
}

Field *getOrCreateField(Position p) {
	// kalo misalnya udah ada di posisi itu maka disimpen aja, kalau misalnya belum ada, buat baru trus return fieldnya aja
	// FieldVaultnya cuma sebagai penanda kalau itu udah pernah ada di linkedlistnya 
	// jadi nanti lebih gampang nyarinya waktu mau ngehubungin posisi yang abstract
	// time complexitynya O(n)
	// dengan n banyak field yang ada
    Field *existingField = findField(p);
    if (!existingField) {
        existingField = createField(p);
		insertNewVault(existingField);
    }
    return existingField;
}

void handleFieldTransition(User *user, char direction) {
	// function ini cuma ketrigger kalau si user melebihi batas dari sebuah field
    Position newFieldPos = user->currentField->p;
    Position newUserPos = user->p;

	resetScreen();
    switch(direction) {
        case 'L':
            newFieldPos.x--;
            newUserPos.x = MAX_WIDTH - 1;
            if(user->currentField->left != NULL){
            	user->currentField = user->currentField->left;
            	user->p = newUserPos;
            	printField(user);
            	return;
			}
            break;
        case 'R':
            newFieldPos.x++;
            newUserPos.x = 0;
            if(user->currentField->right != NULL){
            	user->currentField = user->currentField->right;
            	user->p = newUserPos;
            	printField(user);
            	return;
			}
            break;
        case 'B':
            newFieldPos.y++;
            newUserPos.y = 0;
            if(user->currentField->bottom != NULL){
            	user->currentField = user->currentField->bottom;
            	user->p = newUserPos;
            	printField(user);
            	return;
			}
            break;
        case 'T':
            newFieldPos.y--;
            newUserPos.y = MAX_HEIGHT - 1;
            if(user->currentField->top != NULL){
            	user->currentField = user->currentField->top;
            	user->p = newUserPos;
            	printField(user);
            	return;
			}
            break;
    }
    
    // Kalo misalnya field yang mau didatengin itu null, barulah kita coba cek apakah bisa dipasangin atau ngga
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
    
    printField(user);
}


void displayButtons(int selected) {
    setCursorPosition(8, 20);
    // Highlight "Yes" button if selected == 0
    printf(selected == 0 ? "\033[1;42m   Yes   \033[0m" : "   Yes   ");
    printf("   "); // Space between buttons

    // Highlight "No" button if selected == 1
    printf(selected == 1 ? "\033[1;41m   No   \033[0m" : "   No   ");
}

void printCardType(int type){
	switch(type){
		case 1:
			printf("Attack Card      ");
			break;
		case 2:
			printf("Defense Card     ");
			break;
		case 3:
			printf("Enhance Card     ");
			break;
		case 4:
			printf("Heal Card         ");
			break;
	}
}

void printCardList(Card *card, int cardCount){

	for(int i = 0; i < cardCount; i++){
		if (card[i].type == 1){
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;41m    \033[0m");
			setCursorPosition(4,i*4 + 3);
			printf("\033[1;41m AT \033[0m");
			setCursorPosition(5,i*4 + 3);
			printf("\033[1;41m    \033[0m");
		}
		else if (card[i].type == 2){
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;44m    \033[0m");
			setCursorPosition(4,i*4 + 3);
			printf("\033[1;44m DF \033[0m");
			setCursorPosition(5,i*4 + 3);
			printf("\033[1;44m    \033[0m");
		}
		else if (card[i].type == 3){
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;43m    \033[0m");
			setCursorPosition(4,i*4 + 3);
			printf("\033[1;43m EN \033[0m");
			setCursorPosition(5,i*4 + 3);
			printf("\033[1;43m    \033[0m");
		}
		else if (card[i].type == 4){
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;42m    \033[0m");
			setCursorPosition(4,i*4 + 3);
			printf("\033[1;42m HL \033[0m");
			setCursorPosition(5,i*4 + 3);
			printf("\033[1;42m    \033[0m");
		}
		
	}
	
}


void teleportUserToBase(User *user){
	user->currentField = root;
	user->p.x = MAX_WIDTH/2 - 2;
	user->p.y = MAX_HEIGHT/2 + 2;
}

void winScene(User *user){
	Dungeon currentDungeon = user->currentField->dungeon;
	printMapBox(6, 50);
	setCursorPosition(3, 4);
	printf("YOU have Conquered This Dungeon On Level %d", currentDungeon.baseLevel);
	// cara hitung ice Heart yang didapetin:
	// faktornya cuma user base level sama current Dungeon base level
	int iceHeartAcquired = ((rand() % (user->baseLevel + currentDungeon.baseLevel)) + user->baseLevel)*3;
	user->iceHeartCount += iceHeartAcquired;
	setCursorPosition(4,4);
	printf("You recieved %d Heart Of Ice as the Reward", iceHeartAcquired);
	
	setCursorPosition(5,4);
	if(currentDungeon.baseLevel < 3){
		user->currentField->dungeon.baseLevel++;
		user->currentField->dungeon.enemyBaseAttack *= 2;
		user->currentField->dungeon.enemyCardCount++;
		user->currentField->dungeon.enemyHP *= 2;
		
		user->currentField->dungeon.enemyCard[user->currentField->dungeon.enemyCardCount-1].type = (rand() % 100) < 50 ? 1 : (rand() % 100) < 30 ? 2 : 3;
		printf("Dungeon Level Has Ben Increased....");
	}
	else{
		user->currentField->dungeon.baseLevel++;
		printf("You have fully Conquered This Dungeon Now");
	}
	setCursorPosition(6,4);
	printf("Press Any Key To Continue");
	getch();
}

void gamePlay(User *user, Card *cards){
	resetScreen();
	Dungeon currentDungeon = user->currentField->dungeon;
	// untuk dapetin total turn yang akan ada berdasarkan jumlah kartu terbanyak.
	int userCardCount = user->cardCount;
	int enemyCardCount = currentDungeon.enemyCardCount;
	int totalTurn = enemyCardCount > userCardCount ? enemyCardCount : userCardCount;
	
	// untuk ngatur Jumlah turn yang akan di block attacknya, dan jumlah multiplier untuk kedua pihak
	int userMultiplier = 1;
	int enemyMulitplier = 1;
	int userBlockTurn = 0;
	int enemyBlockTurn = 0;
	int userCurrentHp = user->baseHp;
	int enemyCurrentHp = currentDungeon.enemyHP;
	int winner = 0; // 0 untuk player win, 1 untuk enemy win
	printMapBox(8, 70);
	for(int i = 0; i < totalTurn; i++){
		setCursorPosition(3, 4);
		for(int m = 0; m < 64; m++) printf(" ");
		setCursorPosition(3, 4);
		if(i >= userCardCount){
			printf("%s did nothing", user->name);
		}
		else {
			// Setiap turn
			if(cards[i].type == 1){
				printf("%s used Attack Card", user->name);
				
				int totalDamage = (user->baseAttack * userMultiplier);
				if(rand() % 100 < 15) {
					totalDamage *= 3;
					printf(", CRITICAL!");
				}
				if (enemyBlockTurn > 0){
					totalDamage /= 2;
					enemyBlockTurn--;
					printf(", but the enemy Blocked it");
				}
				enemyCurrentHp -= totalDamage;
				// masukin logic kalah
				userMultiplier = 1;
				setCursorPosition(4,4);
				for(int m = 0; m < 64; m++) printf(" ");
				setCursorPosition(4,4);
				printf("Dealt \033[1;31m%d\033[0m damage to the Enemy", totalDamage);
				
			}
			else if(cards[i].type == 2){
				printf("%s applied Defense Card", user->name);
				setCursorPosition(4,4);
				for(int m = 0; m < 64; m++) printf(" ");
				setCursorPosition(4,4);
				userBlockTurn++;
				printf("User can block for the next \033[1;34m%d\033[0m turn(s)", userBlockTurn);
			}
			else if(cards[i].type == 3){
				printf("%s applied Enhance Card", user->name);
				setCursorPosition(4,4);
				for(int m = 0; m < 64; m++) printf(" ");
				setCursorPosition(4,4);
				userMultiplier *= 2;
				printf("User next attack will be \033[1;33m%dx\033[0m times stronger", userMultiplier);
			}
			else if(cards[i].type == 4){
				printf("%s applied Heal Card", user->name);
				setCursorPosition(4,4);
				for(int m = 0; m < 64; m++) printf(" ");
				setCursorPosition(4,4);
				userCurrentHp += (user->baseHp)/2;
				if(userCurrentHp > user->baseHp) userCurrentHp = user->baseHp;
				printf("Healed user 50%% of max HP");
			}
		}
		setCursorPosition(6,4);
		for(int m = 0; m < 64; m++) printf(" ");
		setCursorPosition(6,4);
		if(i >= enemyCardCount){
			printf("Enemy did nothing");
			setCursorPosition(7,4);
			for(int m = 0; m < 64; m++) printf(" ");
			setCursorPosition(7,4);
		}
		else {
			Card enemyCard = {((rand() % 100) < 50 ? 1 : (rand() % 100) < 30 ? 2 : 3)};

			if(enemyCard.type == 1){
				printf("Enemy used Attack Card");
				
				int totalDamage = (currentDungeon.enemyBaseAttack * enemyMulitplier);
				if (userBlockTurn > 0){
					totalDamage /= 2;
					userBlockTurn--;
					printf(", but %s Blocked half of it", user->name);
				}
				userCurrentHp -= totalDamage;
				// masukin logic user kalah 
				enemyMulitplier = 1;
				setCursorPosition(7,4);
				for(int m = 0; m < 64; m++) printf(" ");
				setCursorPosition(7,4);
				printf("Dealt \033[1;31m%d\033[0m damage to user", totalDamage);
				
			}
			else if(enemyCard.type == 2){
				printf("Enemy applied Defense Card");
				setCursorPosition(7,4);
				for(int m = 0; m < 64; m++) printf(" ");
				setCursorPosition(7,4);
				enemyBlockTurn++;
				printf("Enemy can block for the next \033[1;34m%d\033[0m turn(s)", enemyBlockTurn);
			}
			else if(enemyCard.type == 3){
				printf("Enemy applied Enhance Card");
				setCursorPosition(7,4);
				for(int m = 0; m < 64; m++) printf(" ");
				setCursorPosition(7,4);
				enemyMulitplier *= 2;
				printf("Enemy next attack will be \033[1;33m%dx\033[0m times stronger", enemyMulitplier);
			}
			else if(enemyCard.type == 4){
				printf("Enemy applied Heal Card");
				setCursorPosition(7,4);
				enemyCurrentHp += user->baseHp/2;
				if(enemyCurrentHp > currentDungeon.enemyHP) enemyCurrentHp = currentDungeon.enemyHP;
				printf("Healed 50%% of max HP");
			}
		}
		setCursorPosition(12,4);
		printBlankLine();
		setCursorPosition(12,4);
		printf("User\'s HP: %d | User\'s Multiplier: %d | User\'s block Turn: %d", 
		userCurrentHp < 0 ? 0 : userCurrentHp, userMultiplier, userBlockTurn);
		setCursorPosition(13, 4);
		printBlankLine();
		setCursorPosition(13, 4);
		printf("Enemy\'s HP: %d | Enemy\'s Multiplier: %d | Enemy\'s block Turn: %d", 
		enemyCurrentHp < 0 ? 0 : enemyCurrentHp, enemyMulitplier, enemyBlockTurn);
		Sleep(2000);
		
		if(enemyCurrentHp <= 0 && userCurrentHp > 0){
			// win scene
			flushBuffer();
			winScene(user);
			winner = 0;
			break;
		}
		else if(userCurrentHp <= 0 || (i == totalTurn - 1 && enemyCurrentHp >= 0)){
			// lose scene
			flushBuffer();
			resetScreen();
			setCursorPosition(3, 10);
			printf("\033[1;31mYOU LOST\033[0m");
			setCursorPosition(4,10);
			printf("Press Any Key to Continue");
			getch();
			teleportUserToBase(user);
			winner = 1;
			break;
		}
	}
	// kalau turnnya habis dan enemy belum mati, user otomatis kalah
	
	flushBuffer();
}

void dungeonScene(User *user){
	// sebelum game dimulai, user akan pilih kartu sebanyak max slot kartunya
	// Kartu bakal di random 2 diantara 4 kartu yang tersedia
	resetScreen();
	int userCardCount = user->cardCount;
	Card userCard[userCardCount];
	printMapBox(8, 70);
	setCursorPosition(2,3);
	int dungeonLevel = user->currentField->dungeon.baseLevel;
	printf("%s Dungeon" RESET, dungeonLevel == 1 ? "\033[1;32mEasy" : dungeonLevel == 2 ? "\033[1;33mMedium" : "\033[1;31mHard");
	for(int i = 0; i < userCardCount; i++){
		
		int selected = 0;
		int random1 = (rand() % 100) < 50 ? 1 : (rand() % 100) < 33 ? 2 : (rand() % 100 < 50 ? 3 : 4) ;
		int random2 = (rand() % 100) < 50 ? 1 : (rand() % 100) < 33 ? 2 : (rand() % 100 < 50 ? 3 : 4) ;
		while(1){
			
			
			printCardList(userCard, i);
			
			setCursorPosition(6,3);
			printf("Select one of this card:");
			setCursorPosition(7,3);
			printf("1. "); printCardType(random1); printf("%s", !selected ? " <<< " : "    "); // kalau selected == 0
			setCursorPosition(8,3);
			printf("2. "); printCardType(random2); printf("%s", selected ? " <<< " : "     "); // kalau selected == 1
			char c = getch();
			if(c == 'W' || c == 'w'){
				selected = 0;
				continue;
			}
			else if(c == 'S' || c == 's'){
				selected = 1;
				continue;
			}
			else if(c == '\r'){
				break;
			}
					
		}
		// masukin card type hasil pilih user ke array.
		Card card = {selected ? random2 : random1};
		userCard[i] = card;
	}
	// kelar pemilihan card, masuk ke gameplay
	gamePlay(user, userCard);
	resetScreen();
}

void dungeonEntrance(User *user) {
    resetScreen();
    setCursorPosition(5, 14);
    printf("Do you want to enter this dungeon?");
    
    int selected = 0; 
    displayButtons(selected);

    while (1) {
        char key = _getch(); 

        if (key == 'a' || key == 'A') {  // Move selection left
            selected = 0;
            displayButtons(selected);
        } else if (key == 'd' || key == 'D') {  // Move selection right
            selected = 1;
            displayButtons(selected);
        } else if (key == '\r') {  // Enter key
            break;
        }
    }

    if (selected != 0) {
    	printField(user);
    	printUserPosition(user);
    	return;
	}
    // kalau yang diselect itu yes, berarti masuk ke dungeonnya
    dungeonScene(user);
    
    printField(user);
	printUserPosition(user);
}

void levelUp(User *user, int cost){
	user->baseLevel++;
	user->baseHp *= 2;
	user->baseAttack *= 2;
	user->cardCount++;
	user->iceHeartCount -= cost;
}

void baseMenu(User *user){
	resetScreen();
	int selected = 0;
	int levelUpReq = 10 * pow(5,user->baseLevel-1);
	while(1){
		printMapBox(12, 40);
		setCursorPosition(2,3);
		printf("-Your Base-");
		setCursorPosition(4,3);
		printf("Current Base Level : %d", user->baseLevel);
		setCursorPosition(5,3);
		printf("User HP : %d", user->baseHp);
		setCursorPosition(6,3);
		printf("User Attack : %d", user->baseAttack);
		setCursorPosition(7,3);
		printf("User Card Slot : %d", user->cardCount);
		setCursorPosition(8,3);
		printf("Heart of Ice Owned : %d", user->iceHeartCount);
		setCursorPosition(11,3);
		printf("%s1. Level Up Base (%d)\033[0m", user->iceHeartCount < levelUpReq ? "\033[1;31m" : "", levelUpReq); 
			printf("%s", !selected ? " <<< " : ""); // kalau selected == 0
		setCursorPosition(12,3);
		printf("2. Return"); printf("%s", selected ? " <<< " : ""); 
		
		char c = getch();
		if(c == 'W' || c == 'w'){
			selected = 0;
			continue;
		}
		else if(c == 'S' || c == 's'){
			selected = 1;
			continue;
		}
		else if(c == '\r'){
			if(selected == 0 && user->iceHeartCount >= levelUpReq){
				// validasi kalau heart of ice nya cukup atau ga
				levelUp(user, levelUpReq);
				break;
			}
			if(selected == 1) break;
		}
	}
	resetScreen();
	printField(user);
	printUserPosition(user);
}

void drawTable(int currentPage, int totalPages, int start, int end, int selected, Bookmark **bookmarks) {
    // Header
    setCursorPosition(2,3);
    printf("%-48s", "Bookmarks");

    for (int i = start; i < end; i++) {
    	setCursorPosition(i-start + 4, 3);
        if (i == start + selected) {
            printf("%s%-48s%s", HIGHLIGHT_COLOR, bookmarks[i]->notes, "\033[0m");
        } else {
            printf("%-48s", bookmarks[i]->notes);
        }
    }

    for (int i = end; i < start + 10; i++) {
    	setCursorPosition(end-start + (i - end) + 4, 3);
        printf("%-48s", "");
    }

    // Footer
    setCursorPosition(15,20);
    printf("Page %d of %d",  currentPage + 1, totalPages);
    setCursorPosition(16,1);
    printf("W/S to Navigate");
    setCursorPosition(17,1);
    printf("A/D to Switch Pages");
    setCursorPosition(18,1);
    printf("Press Enter to Select, Esc to Exit");
}

int viewBookmarks(User *user) {
	int PAGE_SIZE = 10;
    int totalBookmarks = 0;
    Bookmark *bookmarks[1000];
    
    // Collect all bookmarks from the hash table
    for (int i = 0; i < 1000; i++) {
        if (hashTable[i] != NULL) {
            bookmarks[totalBookmarks++] = hashTable[i];
        }
    }
    


    int currentPage = 0;
    int selected = 0;
    int totalPages = (totalBookmarks + PAGE_SIZE - 1) / PAGE_SIZE;
    resetScreen();
	printMapBox(12, 50);
    while (1) {
	    if (totalBookmarks == 0) {
	    	// rapiin lagi
	    	printMapBox(2, 50);
	    	setCursorPosition(2,3);
	        printf("No bookmarks available to display.");
	        setCursorPosition(3,3);
	        printf("Press any key to continue.");
	        getch();
	        return 0;
	    }
//        system("cls"); // Clear the screen (Windows)
        
//        setCursorPosition(1, 1);

        int start = currentPage * PAGE_SIZE;
        int end = (start + PAGE_SIZE < totalBookmarks) ? start + PAGE_SIZE : totalBookmarks;

        drawTable(currentPage, totalPages, start, end, selected, bookmarks);

        // Navigation
        char ch = _getch();
        if (ch == 'w' || ch == 'W') {
            selected = (selected - 1 + (end - start)) % (end - start);
        } else if (ch == 's' || ch == 'S') {
            selected = (selected + 1) % (end - start);
        } else if (ch == 'a' || ch == 'A') {
            if (currentPage > 0) {
                currentPage--;
                selected = 0;
            }
        } else if (ch == 'd' || ch == 'D') {
            if (currentPage < (totalBookmarks + PAGE_SIZE - 1) / PAGE_SIZE - 1) {
                currentPage++;
                selected = 0;
            }
        } else if (ch == '\r') { // Enter key
//          printf("Selected Bookmark: (%d) %s\n", start + selected + 1, bookmarks[start + selected]->notes);
//          system("pause");
			Bookmark *selectedBookmark = bookmarks[start + selected];
			user->currentField = selectedBookmark->field;

			
			
			// masukin sub menu dulu, bisa teleport, bisa delete, bisa cancel
			// untuk Select sub menunya
			const char *actions[3] = {
				"1. Teleport       ",
				"2. Delete Bookmark",
				"3. Cancel         "
			};
			int subMenuSelect = 1;
			setCursorPosition(3, 54);
			printf("Select Action: ");
			setCursorPosition(4, 54);
			printf(HIGHLIGHT_COLOR "%s" RESET, actions[0]);
			setCursorPosition(5, 54);
			printf("%s",actions[1]);
			setCursorPosition(6, 54);
			printf("%s",actions[2]);
			
			// looping sub menunya
			while(1){
				char c = getch();
				setCursorPosition(3+subMenuSelect, 54);
				printf("%s", actions[subMenuSelect-1]);
				// untuk validasi dan ngeprint yang bakal di ilangin highlightnya
				if (c == 's' || c == 'S'){
					if(subMenuSelect == 3) subMenuSelect = 1;
					else subMenuSelect++; 
				}
				else if (c == 'w' || c == 'W'){
					if(subMenuSelect == 1) subMenuSelect = 3;
					else subMenuSelect--;
				}
				else if (c == '\r' && subMenuSelect == 1) {
					// Teleport user
					// disini biar bagus, validasi lagi kalau waktu pindah itu gabakal kena dungeonnya
					// logikanya gampang aja, cek kalau dungeonnya lebih dominan di kanan, kita spawn di kolom pertama
					// kalau dungeonnya dominan di kiri, spawn di kolom terakhir
					// karena settingan awalnya kubuat x nya itu diantara 2 sampai 38 jadinya tengahjnya kubuat jadi 20 aja
					if(selectedBookmark->field->dungeon.p.x >= 20){
					// kalau misalnya dominan kanan
						user->p.x = 1;
					}
					else{
						// sisanya berarti kalau dia dominan kiri
						user->p.x = 39;
					}
					return 0;
				}
				else if (c == '\r' && subMenuSelect == 2){
					// delete bookmark
					int result = deleteBookmark(selectedBookmark->notes);
					if(result){
						// dikurangin total bookmarknya, kalau totalnya masih besar 0, print mapboxnya
						totalBookmarks--;
						if(totalBookmarks > 0) printMapBox(12,50);
						else break;
					}
				}
				else if (c == '\r' && subMenuSelect == 3){
					resetScreen();
					printMapBox(12,50);
					break;
				}
				
				
				// untuk override line yang akan di highlighjt
				if (subMenuSelect == 1){
					setCursorPosition(4, 54);
					printf(HIGHLIGHT_COLOR "%s" RESET, actions[subMenuSelect-1]);
				}
				else if(subMenuSelect == 2){
					setCursorPosition(5, 54);
					printf(HIGHLIGHT_COLOR "%s" RESET, actions[subMenuSelect-1]);					
				}
				else if(subMenuSelect == 3){
					setCursorPosition(6, 54);
					printf(HIGHLIGHT_COLOR "%s" RESET, actions[subMenuSelect-1]);
				}
				
			}
        } else if (ch == 27){
        	return 1; // 1 untuk stay in loop menu main
		}
		
	}       
}

void mainMenu(User *user){
    // buat sejenis menu normal aja
	// 1. Teleport To Base
	// 2. Mark As BookMark
	// 3. View Bookmarks
	// 4. Tutorial
	// 5. Return
	// 6. Exit Game
	resetScreen();
	setCursorPosition(2,1);
	puts("\033[1;36m1. Teleport to Base <<<\033[0m");
	puts("2. Mark Location");
	puts("3. View Marked Locations");
	puts("4. Return");
	puts("5. Exit Game");
	
	const char *menuItems[5] = {
        "1. Teleport to Base",
        "2. Mark Location",
        "3. View Marked Locations",
        "4. Return",
        "5. Exit Game"
    };
	
	
	int selected = 0;
    int ch;

    while (1) {
        ch = _getch(); // Get key press without waiting for Enter key

		setCursorPosition(selected+2, 1);
		printBlankLine();
		setCursorPosition(selected+2, 1);
		printf("%s", menuItems[selected]);
        if (ch == 'w' || ch == 'W') {
        	
            selected = (selected - 1 + 5) % 5;

        } else if (ch == 's' || ch == 'S') {
            selected = (selected + 1) % 5;
            
        } else if (ch == '\r') { // Enter key
			if (selected == 0){
				// Teleport to base
				teleportUserToBase(user);
				printMapBox(MAX_HEIGHT, MAX_WIDTH);
				printField(user);
				return;	
			}
			if (selected == 1){
				char note[1000];
				resetScreen();
				
				setCursorPosition(2,1);
				
				printf("Type \'esc\' to return.");
				setCursorPosition(1,1);
				printf("Enter Code/Notes(max 20 Char) : ");
				

				scanf("%[^\n]", note);getchar();
				while(strcmp(note, "esc") != 0 && strlen(note) > 20){
					setCursorPosition(3,1);
					printf(RED "At Most 20 Character" RESET);
					setCursorPosition(2,1);
					printf("Type \'esc\' to return.");
					setCursorPosition(1,1);
					printf("Enter Code/Notes(max 20 Char) : "); printBlankLine();
					setCursorPosition(1,33);
					scanf("%[^\n]", note);getchar();
					if (strcmp(note, "esc") != 0 && strlen(note) <= 20){
						
						break;
					}
				}
				insertBookmark(note, user->currentField);
				return;
			}
			
			if (selected == 2){
				int result = viewBookmarks(user);
				if(result == 1) {
					resetScreen();
					setCursorPosition(2,1);
					selected = 0;
					puts("\033[1;36m1. Teleport to Base <<<\033[0m");
					puts("2. Mark Location");
					puts("3. View Marked Locations");
					puts("4. Return");
					puts("5. Exit Game");
					continue;
				}
				return;
			}
			
//			if (selected == 3){
//				
//				// how to play
//				// In this game you will be spawned on a base field, you can upgrade the base level
//				// Eventually you will also get stronger by leveling up base
//				// You can use the reward recieved from fighting in dungeon to level up base
//				// In the game, you can choose up to certain number of card.
//				// Attack card is to attack the enemy with 100% attack power
//				// Defense card is to block incoming attacks
//				// Enhance card is to strengthen incoming attack, which can be stacked
//				// Heal Card is to heal ourself by 50% of hp
//				
//				// After choosing your cards, you will be facing the enemy and play the card turn by turn
//				// You will always play first. Enemy also have certain number of Card
//				// Dungeon will have 3 level from 1-3. You need to beat level 1 dungeon to make it into level 2
//				// After level 3, the dungeon is considered conquered and you can't enter that dungeon anymore
//				
//				// You can also mark a field with a code or note to make it recognizeable
//				// You can teleport to the marked field by using menu 'q'
//				// You can also remove the mark after you dont need it anymore
//				
//				return;
//			}
			
			if (selected == 3){
				// return ke game
				return;
			}
			
			if (selected == 4){
				exit(0);
			}
			break;
        }
        setCursorPosition(selected+2, 1);
        printf("\033[1;36m%s <<<\033[0m", menuItems[selected]);
    }
	
}

void moveUser(User *user, char input) {
	
	// t untuk top, b untuk bottom, l untuk left, r untuk right
    int newX = user->p.x;
    int newY = user->p.y;
 	Dungeon mapDungeon = user->currentField->dungeon;
	Position currentFieldCoordinate = user->currentField->p;  
	// masing masing ada validasinya untuk cek antara dia ada di base map atau ga, trus kalau misalnya dia di random map,
	// bakal ada collision antara player dengan dungeon
    switch (input) {

        case 'w': case 'W':
        	if (currentFieldCoordinate.x == 0 && currentFieldCoordinate.y == 0){ 
        		// radius x dan dibawah base: gaboleh keatas lagi
        		if ((ABSOLUTE_VALUE(getDiff(user->p.x, (MAX_WIDTH/2) - 2))) < 4
				&& user->p.y == (MAX_HEIGHT/2) + 2){
					// logic base
					baseMenu(user);
        			break;
				}
			}
        	else if (user->p.y == mapDungeon.p.y + 1 &&
			(ABSOLUTE_VALUE(getDiff(user->p.x, mapDungeon.p.x)) <= 1)){
        		if(user->p.x == mapDungeon.p.x && mapDungeon.baseLevel < 4){
        			dungeonEntrance(user);
        			
				}
				break;
				
			}
            if (newY > 0) {
                user->p.y--;
            } else {
                handleFieldTransition(user, 'T');
            }
            break;
        case 's': case 'S':
        	if (currentFieldCoordinate.x == 0 && currentFieldCoordinate.y == 0){ 
        		// radius x dan dibawah base: gaboleh keatas lagi
        		if ((ABSOLUTE_VALUE(getDiff(user->p.x, (MAX_WIDTH/2) - 2))) < 4
				&& user->p.y == (MAX_HEIGHT/2) - 4){
					// logic base
					baseMenu(user);
        			break;
				}
			}
        	else if (user->p.y == mapDungeon.p.y - 2 &&
			(ABSOLUTE_VALUE(getDiff(user->p.x, mapDungeon.p.x)) <= 1)){
				break;
			}
            if (newY < MAX_HEIGHT-1) {
                user->p.y++;
            } else {
                handleFieldTransition(user, 'B');
            }
            break;
        case 'a': case 'A':
        	if (currentFieldCoordinate.x == 0 && currentFieldCoordinate.y == 0){ 
        		// radius x dan dibawah base: gaboleh keatas lagi
        		if (ABSOLUTE_VALUE(getDiff(user->p.y + 1, (MAX_HEIGHT/2))) < 3
				&& user->p.x == (MAX_WIDTH/2) + 2){
					// logic base
					baseMenu(user);
        			break;
				}
			}
        	else if (user->p.x == mapDungeon.p.x + 2 &&
			(getDiff(user->p.y, mapDungeon.p.y) == 0 || getDiff(user->p.y, mapDungeon.p.y) == 1)){
				break;
			}
            if (newX > 0) {
                user->p.x--;
            } else {
                handleFieldTransition(user, 'L');
            }
            break;
        case 'd': case 'D':
        	if (currentFieldCoordinate.x == 0 && currentFieldCoordinate.y == 0){ 
        		// radius x dan dibawah base: gaboleh keatas lagi
        		if (ABSOLUTE_VALUE(getDiff(user->p.y + 1, (MAX_HEIGHT/2))) < 3
				&& user->p.x == (MAX_WIDTH/2) -6){
					//logic base
					baseMenu(user);
        			break;
				}
			}
        	else if (user->p.x == mapDungeon.p.x - 2 &&
			(getDiff(user->p.y, mapDungeon.p.y) == 0 || getDiff(user->p.y, mapDungeon.p.y) == 1)){
				break;
			}
            if (newX < MAX_WIDTH-1) {
                user->p.x++;
            } else {
                handleFieldTransition(user, 'R');
            }
            break;
        case 'm': case 'M':
        	printFieldSummary(user);
        	break;
        	
        case 'q': case 'Q':
        	// buat sejenis menu normal aja
        	// 1. Teleport To Base
        	// 2. Mark As BookMark
        	// 3. View Bookmarks
        	// 4. Tutorial
        	// 5. Return
        	// 6. Exit Game
        	mainMenu(user);
        	// setiap kelar main menu pasti cm bisa balik ke main menu ataupun selesai game
        	// 
        	resetScreen();
        	printMapBox(MAX_HEIGHT, MAX_WIDTH);
        	printField(user);
        	//
    }
}

void moveO(User *user){
   char input;

    // Game loop
    printMapBox(MAX_HEIGHT, MAX_WIDTH);
    printField(user);    
    while (1) {
    	
    	// handle waktu user lagi ada di map doang kan basically/
    	// jadi kalau misalnya gua buat dia print pada saat masuk kesana doang harusnya ga ngaruh
    	// berarti sebenarnya gua cm perlu apply ini di printfield doang, karena yang l,ainnya hrusnya ga terlalu pelru
    	// worst case daeri rencana ini adalah, kita harus nge apply print field tiap saat change
    	// print posisi, setelah kelar input hapus karakter, trus move trus printfield lagi.
           // Print the map and player's position

        input = getch();        // Get the player's input
        setCursorPosition(user->p.y + 2, user->p.x + 2);
        // reset posisi user di UI
        printf(" ");
        moveUser(user, input);  // Update the player's position based on input
		printUserPosition(user);
        
        // waktu handle transition, berarti kita perlu re print mapny 
        // yang perlu di print map box ulang adalah setiap kalikit amasuk scene seperti print field summary, printFrame untuk choose card,
        // print frame untuk gameplay, print frame untuk lost, print frame untuk win, print frame untuk upgrade base
//        if (input == 'q' || input == 'Q') break; // Quit if 'q' is pressed
    }
}

int main() {
	srand(time(NULL));
    HWND consoleWindow = GetConsoleWindow();
    ShowWindow(consoleWindow, SW_MAXIMIZE);
    
    initializeMap();
    
    User *user = (User*)malloc(sizeof(User));
    user->baseLevel = 1;
    user->iceHeartCount = 60;
    user->baseHp = 500;
    user->baseAttack = 100;
    user->cardCount = 8;
    while (1) {
        setCursorPosition(2, 1);
        printf("Insert Name (max 10 characters): ");
        setCursorPosition(3, 1);
        printBlankLine();
        setCursorPosition(3, 1);
        
        char tempName[100];
        scanf("%s", tempName); getchar();

        if (strlen(tempName) > 10) {
            resetScreen();
            setCursorPosition(1, 1);
            printf("Name cannot exceed 10 characters.");
            continue;
        }

        strcpy(user->name, tempName);
        break;
    }
//    strcpy(user->name, "XY");
    teleportUserToBase(user);
    // moveO itu main loop nya
    moveO(user);
    
    return 0;
}
