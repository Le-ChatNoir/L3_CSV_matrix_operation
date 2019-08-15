#include "fonc.h"

/*********** Function ***********/

void initPendingMult(Product * prod){

	size_t i,j;
	for(i=0;i<prod->sizeM1[0];i++)
		{
		for (j=0;j<prod->sizeM2[1];j++){
			prod->pendingMult[i][j]=1;
		}
	}
}

int nbPendingMult(Product * prod)
	{
	size_t i,j;
	int nb=0;
	for(i=0;i<prod->sizeM1[0];i++)
		{
		for (j=0;j<prod->sizeM2[1];j++)
		{
			nb+=prod->pendingMult[i][j];
		}
	}
	return(nb);
}




/*******************************MULT****************************************/
/* 	######### V2 #########
	Chaque thread s'occupe d'une position dans la matrice
	ex : (1,1) (1,2) etc...
	Au début de chaque itération, le thread vérifie s'il doit s'activer.
	Il s'active si 	i<prod.sizeM1[0]
					j<prod.sizeM2[1]	*/

void * mult(void * paramTemp){

	multParam param=*(multParam*)paramTemp;
	int i=param.lresult;
	int j=param.cresult;
	printf("debut mult (%d)(%d) sur le coeur n°%d\n",i,j, sched_getcpu()+1);
	int result=0;


//Attendre l'autorisation de multiplication POUR UNE NOUVELLE ITERATION...
	pthread_mutex_lock(&prod.mutex);
	while(prod.state!=STATE_MULT || prod.pendingMult==0){
		pthread_cond_wait(&prod.cond,&prod.mutex);
	}
	pthread_mutex_unlock(&prod.mutex);

	//Calcul à faire ?
	
	
	// Le premier thread modifie prod.state->WAIT
	
	
	//Calcul 
	int n;
	for(n=0;n<prod.sizeM1[0];n++){
		result+=prod.m1[i-1][n]*prod.m2[n][j-1];
	}
	
	prod.result[i-1][j-1]=result;
	
	prod.pendingMult[i-1][j-1]=0;
	
	// Le dernier thread modifie prod.state->NEXT
	
	if(nbPendingMult(&prod)==0)
	   {
	pthread_mutex_lock(&prod.mutex);
	prod.state=STATE_PRINT;
	pthread_mutex_unlock(&prod.mutex);
	pthread_cond_broadcast(&prod.cond);
		}
}

// Transforme une ligne du tableau en une chaîne de caractères
void getLineMmap(char*file,int nLine,char*retour){
	int curLine=0;
	int i=0;
	int j=0;
	char * line=malloc(sizeof(char)*256);
	
	while(curLine!=nLine){ //A chaque nouvelle boucle on écrase la précédente valeur de line
		strcpy(line,"");
		j=0; //on se met au début de line
		
		while (file[i]!='\n'){	//On écrit chaque caractère
			line[j]=file[i];	//de file jusqu'à rencontrer un \n
			i++;
			j++;
		}
		
		i++;
		line[j+1]='\0'; //On ajoute \0 à la fin de line
		curLine++;
	}
	strcpy(retour,line);
	if (retour==NULL){
		perror("matrice trop grande");
		exit(1);
	}
}

//Transforme une chaine de caractères en un tableau d'entiers

void parseString(char*input,int *tabParsed, int tabSize)
{
	int j;
	char *adrMap_temp1, *adrMap_temp2;
	adrMap_temp1=input;
	
	for(j=0;j<tabSize;j++)
		{
			errno = 0; // pour la vérif de l'erreur
			tabParsed[j]=strtol(adrMap_temp1, &adrMap_temp2, 10);
			if ((errno == ERANGE && (tabParsed[j] == LONG_MAX || tabParsed[j] == LONG_MIN)) || (errno != 0 && tabParsed[j] == 0)|| (errno == 0 && adrMap_temp1==adrMap_temp2))
				{
					fprintf(stderr, "erreur sur le strtol\n");
					exit(EXIT_FAILURE);
				}
			adrMap_temp1 = adrMap_temp2;
		}
}



void setSize(char * file){
	char *adrMap_temp1, *adrMap_temp2;
	adrMap_temp1=file;
	strtol(adrMap_temp1, &adrMap_temp2, 10);
	adrMap_temp1=adrMap_temp2;
	prod.sizeM1[0]=strtol(adrMap_temp1, &adrMap_temp2, 10);
	adrMap_temp1=adrMap_temp2;
	prod.sizeM1[1]=strtol(adrMap_temp1, &adrMap_temp2, 10);
	adrMap_temp1=adrMap_temp2;
	prod.sizeM2[0]=strtol(adrMap_temp1, &adrMap_temp2, 10);
	adrMap_temp1=adrMap_temp2;
	prod.sizeM2[1]=strtol(adrMap_temp1, &adrMap_temp2, 10);
	printf("Taille m1 : (%d %d)\n",prod.sizeM1[0],prod.sizeM1[0]);
	printf("Taille m2 : (%d %d)\n\n",prod.sizeM1[0],prod.sizeM1[0]);
}




void setProduct(char*file){
	//prod.nbIterations=atoi(file[0]);
	int taille[2];
	char buffer[256];
	int cptLigne=4;
	int i=0;
	//On copie la ligne 2 dans un buffer
	printf("Récupération de la taille des matrices...\n");
	setSize(file);
	//On malloc m1
	printf("Allocation de l'espace mémoire des matrices m1, m2 et result...\n\n");
	
	prod.m1=malloc(sizeof(int*)*prod.sizeM1[0]); //prod.*m1
	for (i=0;i<prod.sizeM1[0];i++){
		prod.m1[i]=malloc(sizeof(int)*prod.sizeM1[1]);
	}
	i=0;
	//On malloc m2
	prod.m2=malloc(sizeof(int*)*prod.sizeM2[0]);
	for (i=0;i<prod.sizeM2[0];i++){
		prod.m2[i]=malloc(sizeof(int)*prod.sizeM2[1]);
	}
	i=0;
	//Initialisation de result;
	prod.result=malloc(sizeof(int*)*prod.sizeM1[0]);
	for (i=0;i<prod.sizeM2[0];i++){
		prod.result[i]=malloc(sizeof(int)*prod.sizeM2[1]);
	}
	//Remplissage des matrices
	i=0;
	printf("Remplissage des matrices m1 et m2 à partir du fichier source\n");
	//Remplissage de m1
	while(i<(prod.sizeM1[0])){
		getLineMmap(file,cptLigne,buffer);
		parseString(buffer,prod.m1[i],prod.sizeM1[1]); 
		cptLigne++;
		i++;
	}
	i=0;
	//Remplissage de m2
	while(i<(prod.sizeM2[0])){
		getLineMmap(file,cptLigne,buffer);
		parseString(buffer,prod.m2[i],prod.sizeM2[1]);
		cptLigne++;
		i++;
	}
}



