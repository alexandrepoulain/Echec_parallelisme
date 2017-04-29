#include "projet.h"

/* 2017-02-23 : version 1.0 */

unsigned long long int node_searched = 0;

double my_gettimeofday(){
  struct timeval tmp_time;
  gettimeofday(&tmp_time, NULL);
  return tmp_time.tv_sec + (tmp_time.tv_usec * 1.0e-6L);
}

void evaluate(tree_t * T, result_t *result, int R, int f, int p, MPI_Status status, int tag)
//R : nb de processus restant dispo, f : numero du premier procesus mpi restant dispo, p : numero de processus mpi
//-> cette iteration va distribuer le travail au processus f, f+1, ... , f+R
{
	node_searched++;
  
        move_t moves[MAX_MOVES];
        int n_moves;

        result->score = -MAX_SCORE - 1;
        result->pv_length = 0;
        
        if (test_draw_or_victory(T, result))
          return;

        if (TRANSPOSITION_TABLE && tt_lookup(T, result))     /* la réponse est-elle déjà connue ? */
          return;
        
        compute_attack_squares(T);

        /* profondeur max atteinte ? si oui, évaluation heuristique */
        if (T->depth == 0) {
          result->score = (2 * T->side - 1) * heuristic_evaluation(T);
          return;
        }
        
        n_moves = generate_legal_moves(T, &moves[0]);

        /* absence de coups légaux : pat ou mat */
	if (n_moves == 0) {
          result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
          return;
        }
        
        if (ALPHA_BETA_PRUNING)
          sort_moves(T, n_moves, moves);


	int R2 = (int)R/ n_moves;//nb process / move
	int R3 = R%n_moves;//nb process / move (reste)
	int f2 = f;//copie du premier numero de processus
	
//printf("\tEvaluate #%d %d R : %d R2 : %d R3 : %d f : %d p : %d n_moves : %d \n", p, T->depth, R, R2, R3, f, p, n_moves);
//cas ou plus de processus que de move
	if(R2 >= 1 && R != 1){
printf("\tEvaluate1 #%d %d R : %d R2 : %d R3 : %d f : %d p : %d n_moves : %d \n", p, T->depth, R, R2, R3, f, p, n_moves);
//printf("\t\t#%d cas ou plus de processus que de move\n",p);

	  /* Le result */
	  const int nitems2=4;
	  int          blocklengths2[4] = {1,1,1, MAX_DEPTH};
	  MPI_Datatype types2[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
	  MPI_Datatype mpi_result_t;
	  MPI_Aint     offsets2[4];

	  offsets2[0] = offsetof(result_t, score);
	  offsets2[1] = offsetof(result_t, best_move);
	  offsets2[2] = offsetof(result_t, pv_length);
	  offsets2[3] = offsetof(result_t, PV);

	  MPI_Type_create_struct(nitems2, blocklengths2, offsets2, types2, &mpi_result_t);
	  MPI_Type_commit(&mpi_result_t);
		  
		int supp = 1;
		int* numdespremiersProcess = (int*) malloc(sizeof(int)*n_moves);
		int nbDef = 0;
		/* évalue récursivement les positions accessibles à partir d'ici */
		for (int i = 0; i < n_moves; i++) {
			if(R3==0){
				supp = 0;
			}
				
			if(p>=f && p<f+R2+supp){
				tree_t child;
				result_t child_result;
				
				play_move(T, moves[i], &child);
				
				evaluate(&child, &child_result,R2+supp,f,p,status,tag+1);
				         
				int child_score = -child_result.score;

				if (child_score > result->score) {
					result->score = child_score;
					result->best_move = moves[i];
				        result->pv_length = child_result.pv_length + 1;
				        for(int j = 0; j < child_result.pv_length; j++)
				          result->PV[j+1] = child_result.PV[j];
				          result->PV[0] = moves[i];
				}

				if (ALPHA_BETA_PRUNING && child_score >= T->beta)
				  break;    

				T->alpha = MAX(T->alpha, child_score);
			}
			numdespremiersProcess[nbDef]=f;//sauvegarde des num des premier nouveau process
			nbDef++;//incrementation du compteur de premier process
			f+=R2+supp;//numero du nouveau premier process pour lle prochain appel d evaluate
			R3--;//desincrementation du reste
			//numdespremiersProcess[i]=f;
		}

		if (TRANSPOSITION_TABLE)
		  tt_store(T, result);
		
		if(p==f2){
		//reception des result
			for (int i = 1; i < nbDef; i++) {
				result_t child_result;
				MPI_Recv(&child_result, 1, mpi_result_t, numdespremiersProcess[i], tag, MPI_COMM_WORLD, &status);
				int child_score = -child_result.score;
				if (child_score > result->score){
					result->score = child_score;
					//result->best_move = moves[i];
					result->best_move = child_result.best_move;
					result->pv_length = child_result.pv_length + 1;
					for(int j = 0; j < child_result.pv_length; j++)
						result->PV[j+1] = child_result.PV[j];
					//result->PV[0] = moves[i];
					result->PV[0] = child_result.best_move;
				}
			}
		}else{
			for(int j = 1; j< nbDef;j++){
				if(p == numdespremiersProcess[j]){
				//envoie des result
					MPI_Send(result, 1, mpi_result_t, f2, tag, MPI_COMM_WORLD);
				}	
			}
		}
		 
	}
	
//cas ou moins de processus que de move
	if(R2 < 1 && R != 1){
printf("\tEvaluate2 #%d %d R : %d R2 : %d R3 : %d f : %d p : %d n_moves : %d \n", p, T->depth, R, R2, R3, f, p, n_moves);
//printf("\t\t#%d cas ou moins de processus que de move\n", p);

	  /* Le result */
	  const int nitems2=4;
	  int          blocklengths2[4] = {1,1,1, MAX_DEPTH};
	  MPI_Datatype types2[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
	  MPI_Datatype mpi_result_t;
	  MPI_Aint     offsets2[4];

	  offsets2[0] = offsetof(result_t, score);
	  offsets2[1] = offsetof(result_t, best_move);
	  offsets2[2] = offsetof(result_t, pv_length);
	  offsets2[3] = offsetof(result_t, PV);

	  MPI_Type_create_struct(nitems2, blocklengths2, offsets2, types2, &mpi_result_t);
	  MPI_Type_commit(&mpi_result_t);

	      /* évalue récursivement les positions accessibles à partir d'ici */
		for (int i = 0; i < n_moves; i++) {
			if((i%R)+f2==p){
				tree_t child;
				result_t child_result;
				
				play_move(T, moves[i], &child);			
				evaluate(&child, &child_result,1,p,p,status,tag+1);			         
				int child_score = -child_result.score;

				if (child_score > result->score) {
					result->score = child_score;
					result->best_move = moves[i];
				        result->pv_length = child_result.pv_length + 1;
				        for(int j = 0; j < child_result.pv_length; j++)
				          result->PV[j+1] = child_result.PV[j];
				          result->PV[0] = moves[i];
				}

				if (ALPHA_BETA_PRUNING && child_score >= T->beta)
				  break;    

				T->alpha = MAX(T->alpha, child_score);
			}
		}

		if (TRANSPOSITION_TABLE)
		  tt_store(T, result);
		if(p==f2){
		//reception des result
			for (int i = f2+1; i < R+f2; i++) {
				result_t child_result;
				MPI_Recv(&child_result, 1, mpi_result_t, i, tag, MPI_COMM_WORLD, &status);
				int child_score = -child_result.score;
				if (child_score > result->score){
					result->score = child_score;
					//result->best_move = moves[i];
					result->best_move = child_result.best_move;
					result->pv_length = child_result.pv_length + 1;
					for(int j = 0; j < child_result.pv_length; j++){
						result->PV[j+1] = child_result.PV[j];
					}
					//result->PV[0] = moves[i];
					result->PV[0] = child_result.best_move;
				}
			}
		}else{
		//envoie des result
			MPI_Send(result, 1, mpi_result_t, f2, tag, MPI_COMM_WORLD);
		}
	}
//cas ou 1 seul processus  -> evaluate normale
	if(R2 == 0 && R == 1){
printf("\tEvaluate #%d %d R : %d R2 : %d R3 : %d f : %d p : %d n_moves : %d \n", p, T->depth, R, R2, R3, f, p, n_moves);
//printf("\t\t#%d cas ou 1 seul processus  -> evaluate normale\n", p);
        /* évalue récursivement les positions accessibles à partir d'ici */
		for (int i = 0; i < n_moves; i++) {
			tree_t child;
		        result_t child_result;
		        
		        play_move(T, moves[i], &child);
		        
		        evaluate(&child, &child_result,1,p,p,status,tag+1);
		                 
		        int child_score = -child_result.score;

			if (child_score > result->score) {
				result->score = child_score;
				result->best_move = moves[i];
		                result->pv_length = child_result.pv_length + 1;
		                for(int j = 0; j < child_result.pv_length; j++)
		                  result->PV[j+1] = child_result.PV[j];
		                  result->PV[0] = moves[i];
		        }

		        if (ALPHA_BETA_PRUNING && child_score >= T->beta)
		          break;    

		        T->alpha = MAX(T->alpha, child_score);
		}

		if (TRANSPOSITION_TABLE)
		  tt_store(T, result);
	  }
//printf("\tFIN Evaluate #%d %d  result->score : %d \n", p, T->depth, result->score);
}


void decide(tree_t * T, result_t *result, int p, int np, MPI_Status status, int tag)
{
	//printf("decide p : %d np : %d\n",p, np);
	for (int depth = 1;; depth++) {
		int S;
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;
		if(p==0)
                	printf("=====================================\n");
		evaluate(T, result,np,0,p,status,tag);
printf("mtn %d\n",p);	
int ig;
scanf("%d", &ig);
		if(p==0){
		        printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
		        print_pv(T, result);
		        if (DEFINITIVE(result->score)){ //send finalize
		          S  =  0;
	        	  tag = 0;
	        	  for(int i = 1;i<np;i++) 
	        	  	MPI_Send(&S, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
		          break;
	          	}else{ //send continue
	          	  S  = 1;
	          	  tag = 0;
	          	  for(int i = 1;i<np;i++) 
	          	  	MPI_Send(&S, 1, MPI_INT, i, tag, MPI_COMM_WORLD);
	          	  tag = 10;
	          	}        
               	}else{
               		//receive on f quoi ensuite?
               		MPI_Recv(&S, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
               		if(S == 0)
       			  break;
		  	tag = 10;
               	}
	}
}

int main(int argc, char **argv)
{  
	  
  /* Init MPI */
  int NP, rang, tag = 10;
  int provided;
  MPI_Init(&argc,&argv);
  //MPI_Init_thread(&argc,&argv, MPI_THREAD_MULTIPLE, &provided);
  // nombre de processus
  
  MPI_Comm_size(MPI_COMM_WORLD, &NP);
  // Le rang des processus
  MPI_Comm_rank(MPI_COMM_WORLD, &rang);
  // le status
  MPI_Status status; 
	
	
	tree_t root;
	result_t result;
	double debut, fin;

	if (argc < 2) {
	  printf("usage: %s \"4k//4K/4P w\" (or any position in FEN)\n", argv[0]);
	  exit(1);
	}

	if (ALPHA_BETA_PRUNING)
	  printf("Alpha-beta pruning ENABLED\n");

	if (TRANSPOSITION_TABLE) {
	  printf("Transposition table ENABLED\n");
	  init_tt();
	}

	parse_FEN(argv[1], &root);
  if(rang==0){
	print_position(&root);
 /* debut du chronometrage */
  debut = my_gettimeofday();   
  } 
	decide(&root, &result,rang, NP, status,tag);
  if(rang==0){
  /* fin du chronometrage */
  fin = my_gettimeofday();
  fprintf( stderr, "Temps total de calcul : %g sec\n", 
     fin - debut);
  fprintf( stdout, "%g\n", fin - debut);
  
	printf("\nDécision de la position: ");
        switch(result.score * (2*root.side - 1)) {
        case MAX_SCORE: printf("blanc gagne\n"); break;
        case CERTAIN_DRAW: printf("partie nulle\n"); break;
        case -MAX_SCORE: printf("noir gagne\n"); break;
        default: printf("BUG\n");
        }

        printf("Node searched: %llu\n", node_searched);
  }
        if (TRANSPOSITION_TABLE)
          free_tt();
        MPI_Finalize();
	return 0;
}
