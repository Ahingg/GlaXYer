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
#define ABSOLUTE_VALUE(x) ((x) < 0 ? -(x) : (x))

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
		newField->dungeon.p.x = (rand() % 37) + 2;
		newField->dungeon.p.y = (rand() % 17) + 2;
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
		YELLOW : RED);
		printf("%c%c%c" RESET, 220, 220, 220);
		
		setCursorPosition(2 + dungeonPosition.y, 1 + dungeonPosition.x);
		printf(user->currentField->dungeon.baseLevel < 2 ? GREEN :
		(user->currentField->dungeon.baseLevel < 3) ?
		YELLOW : RED);
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
			setCursorPosition(2,i*4 + 3);
			printf("\033[1;41m    \033[0m");
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;41m AT \033[0m");
			setCursorPosition(4,i*4 + 3);
			printf("\033[1;41m    \033[0m");
		}
		else if (card[i].type == 2){
			setCursorPosition(2,i*4 + 3);
			printf("\033[1;44m    \033[0m");
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;44m DF \033[0m");
			setCursorPosition(4,i*4 + 3);
			printf("\033[1;44m    \033[0m");
		}
		else if (card[i].type == 3){
			setCursorPosition(2,i*4 + 3);
			printf("\033[1;43m    \033[0m");
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;43m EN \033[0m");
			setCursorPosition(4,i*4 + 3);
			printf("\033[1;43m    \033[0m");
		}
		else if (card[i].type == 4){
			setCursorPosition(2,i*4 + 3);
			printf("\033[1;42m    \033[0m");
			setCursorPosition(3,i*4 + 3);
			printf("\033[1;42m HL \033[0m");
			setCursorPosition(4,i*4 + 3);
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
	int iceHeartAcquired = (rand() % (user->baseLevel + currentDungeon.baseLevel))*3;
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
		printf("You have fully Conquered This Dungeon Now");
	}
	setCursorPosition(6,4);
	printf("Press Any Key To Continue");
	

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
	printMapBox(8, 60);
	for(int i = 0; i < totalTurn; i++){
		
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
				printf("Dealt \033[1;31m%d\033[0m damage to the Enemy", totalDamage);
				
			}
			else if(cards[i].type == 2){
				printf("%s applied Defense Card", user->name);
				setCursorPosition(4,4);
				userBlockTurn++;
				printf("User can block for the next \033[1;34m%d\033[0m turn(s)", userBlockTurn);
			}
			else if(cards[i].type == 3){
				printf("%s applied Enhance Card", user->name);
				setCursorPosition(4,4);
				userMultiplier *= 2;
				printf("User next attack will be \033[1;33m%dx\033[0m times stronger", userMultiplier);
			}
			else if(cards[i].type == 4){
				printf("%s applied Heal Card", user->name);
				setCursorPosition(4,4);
				userCurrentHp += (user->baseHp)/2;
				if(userCurrentHp > user->baseHp) userCurrentHp = user->baseHp;
				printf("Healed user 50%% of max HP");
			}
		}
		setCursorPosition(6, 4);
		if(i >= enemyCardCount){
			printf("Enemy did nothing");
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
				printf("Dealt \033[1;31m%d\033[0m damage to user", totalDamage);
				
			}
			else if(enemyCard.type == 2){
				printf("Enemy applied Defense Card");
				setCursorPosition(7,4);
				enemyBlockTurn++;
				printf("Enemy can block for the next \033[1;34m%d\033[0m turn(s)", enemyBlockTurn);
			}
			else if(enemyCard.type == 3){
				printf("Enemy applied Enhance Card");
				setCursorPosition(7,4);
				enemyMulitplier *= 2;
				printf("Enemy next attack will be \033[1;33m%dx\033[0m times stronge ", enemyMulitplier);
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
		printf("User\'s HP: %d | User\'s Multiplier: %d | User\'s block Turn: %d", 
		userCurrentHp < 0 ? 0 : userCurrentHp, userMultiplier, userBlockTurn);
		setCursorPosition(13, 4);
		printf("Enemy\'s HP: %d | Enemy\'s Multiplier: %d | Enemy\'s block Turn: %d", 
		enemyCurrentHp < 0 ? 0 : enemyCurrentHp, enemyMulitplier, enemyBlockTurn);
		Sleep(2000);
		
		if(enemyCurrentHp <= 0 && userCurrentHp > 0){
			// win scene
			winScene(user);
			winner = 0;
			break;
		}
		else if(userCurrentHp <= 0){
			// lose scene
			resetScreen();
			setCursorPosition(3, 10);
			printf("\033[1;31mYOU LOST\033[0m");
			setCursorPosition(4,10);
			printf("Press Any Key to Continue");
			teleportUserToBase(user);
			winner = 1;
			break;
		}
	}
	// kalau turnnya habis dan enemy belum mati, user otomatis kalah
	
	getch();
}

void dungeonScene(User *user){
	// sebelum game dimulai, user akan pilih kartu sebanyak max slot kartunya
	// Kartu bakal di random 2 diantara 4 kartu yang tersedia
	resetScreen();
	int userCardCount = user->cardCount;
	Card userCard[userCardCount];
	printMapBox(7, 70);
	for(int i = 0; i < userCardCount; i++){
		
		int selected = 0;
		int random1 = (rand() % 100) < 50 ? 1 : (rand() % 100) < 33 ? 2 : (rand() % 100 < 50 ? 3 : 4) ;
		int random2 = (rand() % 100) < 50 ? 1 : (rand() % 100) < 33 ? 2 : (rand() % 100 < 50 ? 3 : 4) ;
		while(1){
			
			
			printCardList(userCard, i);
			
			setCursorPosition(5,2);
			printf("Select one of this card:");
			setCursorPosition(6,2);
			printf("1. "); printCardType(random1); printf("%s", !selected ? " <<< " : "    "); // kalau selected == 0
			setCursorPosition(7,2);
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
		printMapBox(10, 30);
		setCursorPosition(2,2);
		printf("Current Base Level : %d", user->baseLevel);
		setCursorPosition(3,2);
		printf("User HP : %d", user->baseHp);
		setCursorPosition(4,2);
		printf("User Attack : %d", user->baseAttack);
		setCursorPosition(5,2);
		printf("User Card Slot : %d", user->cardCount);
		setCursorPosition(7,2);
		printf("%s1. Level Up Base (%d)\033[0m", user->iceHeartCount < levelUpReq ? "\033[1;31m" : "", levelUpReq); 
			printf("%s", !selected ? " <<< " : ""); // kalau selected == 0
		setCursorPosition(8,2);
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
	puts("4. How To Play");
	puts("5. Return");
	puts("6. Exit Game");
	
	const char *menuItems[6] = {
        "1. Teleport to Base",
        "2. Mark Location",
        "3. View Marked Locations",
        "4. How To Play",
        "5. Return",
        "6. Exit Game"
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
        	
            selected = (selected - 1 + 6) % 6;

        } else if (ch == 's' || ch == 'S') {
            selected = (selected + 1) % 6;
            
        } else if (ch == '\r') { // Enter key
//            switch (selected) {
//                case 0:
//                    printf("\nTeleporting to Base...\n");
//                    break;
//                case 1:
//                    printf("\nMarking Location...\n");
//                    break;
//                case 2:
//                    printf("\nViewing Marked Locations...\n");
//                    break;
//                case 3:
//                    printf("\nShowing How To Play...\n");
//                    break;
//                case 4:
//                    printf("\nReturning to Previous Menu...\n");
//                    break;
//                case 5:
//                    printf("\nExiting Game...\n");
//                    exit(0);
//                    break;
//            }
//            Sleep(1000);
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
        		if(user->p.x == mapDungeon.p.x){
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
    user->iceHeartCount = 10;
    user->baseHp = 500;
    user->baseAttack = 100;
    user->cardCount = 8;
    strcpy(user->name, "XY");
    teleportUserToBase(user);
    
    moveO(user);
    
    return 0;
}
