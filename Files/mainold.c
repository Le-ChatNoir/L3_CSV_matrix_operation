int main(int argc,char ** argv){
	size_t i, iter;
	pthread_t *multTh;
	size_t    *multData;
	pthread_t  addTh;
	void      *threadReturnValue;



	/* Lire le nombre d'iterations et la taille des vecteurs */

	if((argc<=2)||
	   (sscanf(argv[1],"%u",(unsigned int *)&prod.nbIterations)!=1)||
	   (sscanf(argv[2],"%u",(unsigned int *)&prod.size)!=1)||
	   ((int)prod.nbIterations<=0)||((int)prod.size<=0))
	  {
	  fprintf(stderr,"usage: %s nbIterations vectorSize\n",argv[0]);
	  return(EXIT_FAILURE);
	  }

	/* Initialisations (Product, tableaux, generateur aleatoire,etc) */
	prod.state=STATE_WAIT;
	pthread_cond_broadcast(&prod.cond);

	prod.pendingMult=(int *)malloc(prod.size*sizeof(int));
	initPendingMult(&prod);

	/*initialiser prod.mutex ... */
	if(pthread_mutex_init(&prod.mutex,NULL)!=0){
		perror("mutex");
		exit(1);
	}
	/*initialiser prod.cond ... */
	if(pthread_cond_init(&prod.cond,NULL)!=0){
		perror("cond");
		exit(2);
	}

	/* Allocation dynamique des 3 vecteurs v1, v2, v3 */
	prod.v1=(double *)malloc(prod.size*sizeof(double));
	prod.v2=(double *)malloc(prod.size*sizeof(double));
	prod.v3=(double *)malloc(prod.size*sizeof(double));
	/* Allocation dynamique du tableau pour les threads multiplieurs */

	multTh=(pthread_t *)malloc(prod.size*sizeof(pthread_t));
	/* Allocation dynamique du tableau des MulData */

	multData=(size_t *)malloc(prod.size*sizeof(size_t));
	/* Initialisation du tableau des MulData */

	for(i=0;i<prod.size;i++)
	{
	 multData[i]=i;
	}

	/*---------------------------------CREATION DE L'ENSEMBLE----------------------------------------*/

	int nbCore = sysconf(_SC_NPROCESSORS_ONLN);
	printf("Nombre de coeurs disponibles: %d\n\n", nbCore);
	cpu_set_t ensemble;
	CPU_ZERO(&ensemble);

	pthread_t idThread;



	/*Creer les threads de multiplication... --------------------------------------------------------- */
	for(i=0;i<prod.size;i++){
		CPU_SET((i%nbCore), &ensemble);
		if(pthread_create(&multTh[i],NULL,mult,&multData[i])!=0){
			perror("threadMult");
			exit(3);
		}
		/*------------AFFECTATION DES THREADS AUX COEURS--------------*/
		idThread=multTh[i];
		pthread_setaffinity_np(idThread, sizeof(cpu_set_t), &ensemble); //gettid ?
		CPU_ZERO(&ensemble);

	}

	/*Creer le thread d'addition...          */
	if(pthread_create(&addTh,NULL,add,&prod.result)!=0){
		perror("threadAdd");
		exit(3);
	}

	srand(time((time_t *)0));   /* Init du generateur de nombres aleatoires */
	/* Pour chacune des iterations a realiser, c'est a dire :                   */
	for(iter=0;iter<prod.nbIterations;iter++) /* tant que toutes les iterations */
	  {                                       /* n'ont pas eu lieu              */
	  size_t j;
	 
	  /* Initialiser aleatoirement les deux vecteurs */
	  for(j=0;j<prod.size;j++)
		{
		prod.v1[j]=10.0*(0.5-((double)rand())/((double)RAND_MAX));
		prod.v2[j]=10.0*(0.5-((double)rand())/((double)RAND_MAX));
		}

	  /*Autoriser le demarrage des multiplications pour une nouvelle iteration..*/
		pthread_mutex_lock(&prod.mutex);
		prod.state=STATE_MULT;
		pthread_mutex_unlock(&prod.mutex);
		pthread_cond_broadcast(&prod.cond);

	  /*=>Attendre l'autorisation d'affichage...*/
		pthread_mutex_lock(&prod.mutex);
		while(prod.state!=STATE_PRINT){
			pthread_cond_wait(&prod.cond,&prod.mutex);
		}
		pthread_mutex_unlock(&prod.mutex);

	  /*=>Afficher le resultat de l'iteration courante...*/	
		printf("v1.v2=%f\n",prod.result);
	  }

	/*=>Attendre la fin des threads de multiplication...*/
	for(i=0;i<prod.size;i++){
		pthread_join(multTh[i],&threadReturnValue);
	}

	/*Attendre la fin du thread d'addition...*/
	pthread_join(addTh,&threadReturnValue);

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



	/* Detruire avec free ce qui a ete initialise avec malloc */

	free(prod.pendingMult);
	free(prod.v1);
	free(prod.v2);
	free(prod.v3);
	free(multTh);
	free(multData);
	return(EXIT_SUCCESS);

}
