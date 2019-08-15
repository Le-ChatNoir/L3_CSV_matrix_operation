#include "fonc.h"

int main(int argc,char ** argv){
	int i,j;
	pthread_t *multTh;
	multParam **multData;
	char * file;
	multParam iteration1;
	void      *threadReturnValue;


	if(argc != 2)
		{
		printf("Utilisation du programme: %s [Path du fichier de données]\n", argv[0]);	
		exit(0);
		}

		/*------------------MMAP-------------------*/
	int fd = open(argv[1], O_RDWR, 0777);
	printf("Fichier ouvert\n\n");
		
	file = (char *) mmap(NULL, 10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	pid_t pidmmap = getpid();
	printf("Liaison mmap créée\nProcessus du mmap: %d\n\n", pidmmap);
	

	setProduct(file); //Remplissage de la structure prod avec la première itération
	
	//affichage matrices
	printf("\nAffichage M1 :\n");
	for(i=0;i<prod.sizeM1[0];i++){
		printf("( ");
		for(j=0;j<prod.sizeM1[1];j++){
			printf("%d ",prod.m1[i][j]);
		}
		printf(")\n");
	}
	printf("\nAffichage M2 :\n");
	for(i=0;i<prod.sizeM2[0];i++){
		printf("( ");
		for(j=0;j<prod.sizeM2[1];j++){
			printf("%d ",prod.m2[i][j]);
		}
		printf(")\n");
	}
	printf("\n");
	
	// Initialisations (tableaux) 
	prod.state=STATE_WAIT;
	pthread_cond_broadcast(&prod.cond);


	//Initialisation pending mult
	prod.pendingMult=malloc(prod.sizeM1[0]*sizeof(int*));
	for (i=0;i<prod.sizeM1[0];i++){
		prod.pendingMult[i]=malloc(prod.sizeM2[1]*sizeof(int));
	}
	
	initPendingMult(&prod);

	//initialiser prod.mutex ... 
	if(pthread_mutex_init(&prod.mutex,NULL)!=0){
		perror("mutex");
		exit(1);
	}
	//initialiser prod.cond ... 
	if(pthread_cond_init(&prod.cond,NULL)!=0){
		perror("cond");
		exit(2);
	}

	// Allocation dynamique du tableau pour les threads multiplieurs 
	printf("Allocation du tableau de threads...\n");
	multTh=(pthread_t *)malloc((prod.sizeM1[0]*prod.sizeM2[1])*sizeof(pthread_t));
	
	// Allocation dynamique du tableau des MulData
	multData=malloc(prod.sizeM1[0]*sizeof(multParam*));
	for (i=0;i<prod.sizeM1[0];i++){
		multData[i]=malloc(prod.sizeM2[1]*sizeof(multParam));
	}
	
	//Remplissage du tableau des multData
	printf("Remplissage du tableau multdata\n");
	for(i=0;i<prod.sizeM1[0];i++){
		for(j=0;j<prod.sizeM2[1];j++){
			multData[i][j].lresult=i+1;
			multData[i][j].cresult=j+1;
		}
	}
	
	
	// AFFICHAGE DES MULTS DATA POUR TEST
	for(i=0;i<prod.sizeM1[0];i++){
		for(j=0;j<prod.sizeM2[1];j++){
			printf("multDATA (%d %d) : (%d %d)\n",i,j,multData[i][j].lresult,multData[i][j].cresult);
		}
	}
	
	
	
//---------------------------------CREATION DE L'ENSEMBLE----------------------------------------

	int nbCore = sysconf(_SC_NPROCESSORS_ONLN);
	printf("\nNombre de coeurs disponibles: %d\n\n", nbCore);
	cpu_set_t ensemble;
	CPU_ZERO(&ensemble);

	pthread_t idThread;
	
	
	
//Creer les threads de multiplication... ---------------------------------------------------------
	printf("Creation des threads...\n"); 
	printf("\nAffectation des threads aux coeurs...\n");
	for(i=0;i<prod.sizeM1[0];i++){
		for(j=0;j<prod.sizeM2[1];j++){
			CPU_SET((i%nbCore), &ensemble);
			if(pthread_create(&multTh[i],NULL,mult,&multData[i][j])!=0){
				perror("threadMult");
				exit(3);
			}
		
			//------------AFFECTATION DES THREADS AUX COEURS--------------
			idThread=multTh[i];
			pthread_setaffinity_np(idThread, sizeof(cpu_set_t), &ensemble);
			CPU_ZERO(&ensemble);
		}

	}
	
	//Autoriser le demarrage des multiplications pour une nouvelle iteration (POUR V2)
	pthread_mutex_lock(&prod.mutex);
	prod.state=STATE_MULT;
	pthread_mutex_unlock(&prod.mutex);
	pthread_cond_broadcast(&prod.cond);
	
	//=>Attendre l'autorisation d'affichage...
	pthread_mutex_lock(&prod.mutex);
	while(prod.state!=STATE_PRINT){
		pthread_cond_wait(&prod.cond,&prod.mutex);
	}
	pthread_mutex_unlock(&prod.mutex);
	
	
	printf("\nAFFICHAGE DU RESULTAT :\n");

	for(i=0;i<prod.sizeM1[0];i++){
		printf("( ");
		for(j=0;j<prod.sizeM1[1];j++){
			printf("%d ",prod.result[i][j]);
		}
		printf(")\n");
	}
	printf("\n");
	
	
		/*=> detruire prod.cond ... */
	if(pthread_mutex_destroy(&prod.mutex)!=0){
		perror("mutexDestroy");
		exit(4);
		}
	/*=> detruire prod.mutex ... */
	if(pthread_cond_destroy(&prod.cond)!=0){
		perror("condDestroy");
		exit(5);
		}
	
	
	free(prod.pendingMult);
	free(prod.m1);
	free(prod.m2);
	free(prod.result);
	free(multTh);
	free(multData);
	
	
	

	return(0);

}
