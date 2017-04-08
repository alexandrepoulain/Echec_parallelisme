#include "projet.h"

/* 2017-02-23 : version 1.0 */

unsigned long long int node_searched = 0;

double my_gettimeofday(){
  struct timeval tmp_time;
  gettimeofday(&tmp_time, NULL);
  return tmp_time.tv_sec + (tmp_time.tv_usec * 1.0e-6L);
}

void evaluate(tree_t * T, result_t *result, int rang)
{
        //Maitre
        if(rang==0){
        MPI_Status status;
        
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
		printf("n_moves %d\n",n_moves);
		/* absence de coups légaux : pat ou mat */
		if (n_moves == 0) {
		  result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
		  return;
		}
		
		if (ALPHA_BETA_PRUNING)
		  sort_moves(T, n_moves, moves);
		//nb de process
		int p;
		MPI_Comm_size(MPI_COMM_WORLD, &p);
		
		//Initialisation: envoie des premiers travaux
		int cptEnvoie = n_moves;
		int cptRecue = 0;
		for(int i=0; i<p-1; i++){
			if(cptEnvoie==0){
				break;
			}else{
				tree_t child;
				play_move(T, moves[n_moves-cptEnvoie], &child);
				//envoie de T
				int Tr[534];
				for(int j=0;j<128;j++){
					Tr[j]=(int)child.pieces[j];
				}
				for(int j=128;j<256;j++){
					Tr[j]=(int)child.colors[j];
				}Tr[257]=child.side; 	
				Tr[258]=child.depth;
				Tr[259]=child.height;
				Tr[260]=child.alpha;
				Tr[270]=child.beta;
				Tr[271]=child.alpha_start;
				Tr[272]=child.king[0];
				Tr[273]=child.king[1];
				Tr[274]=child.pawns[0];
				Tr[275]=child.pawns[1];					
				for(int j=276;j<404;j++){
					Tr[j]=(int)child.attack[j];
				}
				Tr[404]=child.suggested_move;
				Tr[405]=(int)child.suggested_move;
				for(int j=406;j<534;j++){
					Tr[j]=child.history[j];
				}
				Tr[534]=n_moves-cptEnvoie;		
				printf("Maitre envoie a %d \n",i+1);
				MPI_Send(&Tr, 535, MPI_INT, i+1, TAG_REQ, MPI_COMM_WORLD); 
				printf("Maitre envoié a %d \n",i+1);
				cptEnvoie--;
			}
		}

		//envoie reception des travaux suivant
		while(cptEnvoie > 0){
			//reception de result
			result_t child_result;
			int R[MAX_DEPTH + 4];
			MPI_Recv(R, MAX_DEPTH + 4, MPI_INT, status.MPI_SOURCE, TAG_DATA, MPI_COMM_WORLD, &status);
			child_result.score=R[0];
			child_result.best_move=R[1];
			child_result.pv_length=R[2];
			for(int i = 3;i<MAX_DEPTH;i++){
				child_result.PV[i-3]=R[i];
			}
			int indiceMove = R[MAX_DEPTH + 3];
			cptRecue++;
			
			int child_score = -child_result.score;

			if (child_score > result->score) {
				result->score = child_score;
				result->best_move = moves[indiceMove];
		                result->pv_length = child_result.pv_length + 1;
		                for(int j = 0; j < child_result.pv_length; j++)
		                  result->PV[j+1] = child_result.PV[j];
		                  result->PV[0] = moves[indiceMove];
		        }

			//part 2
		        if (ALPHA_BETA_PRUNING && child_score >= T->beta)
		          break;    

		        T->alpha = MAX(T->alpha, child_score);
			
			tree_t child;
			play_move(T, moves[n_moves-cptEnvoie], &child);
			//envoie de T
			int Tr[534];
			for(int j=0;j<128;j++){
				Tr[j]=(int)child.pieces[j];
			}
			for(int j=128;j<256;j++){
				Tr[j]=(int)child.colors[j];
			}Tr[257]=child.side; 	
			Tr[258]=child.depth;
			Tr[259]=child.height;
			Tr[260]=child.alpha;
			Tr[270]=child.beta;
			Tr[271]=child.alpha_start;
			Tr[272]=child.king[0];
			Tr[273]=child.king[1];
			Tr[274]=child.pawns[0];
			Tr[275]=child.pawns[1];					
			for(int j=276;j<404;j++){
				Tr[j]=(int)child.attack[j];
			}
			Tr[404]=child.suggested_move;
			Tr[405]=(int)child.suggested_move;
			for(int j=406;j<534;j++){
				Tr[j]=child.history[j];
			}			
			Tr[534]=n_moves-cptEnvoie;			
			MPI_Send(&Tr, 535, MPI_INT, status.MPI_SOURCE, TAG_REQ, MPI_COMM_WORLD);  
			cptEnvoie--;
			
		}
		printf("maitre init fait\n");
		//reception des derniers travaux
		while(cptRecue < n_moves){
			//reception de result
			result_t child_result;
			int R[MAX_DEPTH + 4];
			MPI_Recv(R, MAX_DEPTH + 4, MPI_INT, status.MPI_SOURCE, TAG_DATA, MPI_COMM_WORLD, &status);
			printf("recu travail fait de %d\n",status.MPI_SOURCE);
			child_result.score=R[0];
			child_result.best_move=R[1];
			child_result.pv_length=R[2];
			for(int i = 3;i<MAX_DEPTH;i++){
				child_result.PV[i-3]=R[i];
			}
			int indiceMove = R[MAX_DEPTH + 3];		
			cptRecue++;
			
			//envoie fin
			int Tr[534];;
			MPI_Send(Tr, 534, MPI_INT, status.MPI_SOURCE, TAG_END, MPI_COMM_WORLD);
			
			int child_score = -child_result.score;

			if (child_score > result->score) {
				result->score = child_score;
				result->best_move = moves[indiceMove];
		                result->pv_length = child_result.pv_length + 1;
		                for(int j = 0; j < child_result.pv_length; j++)
		                  result->PV[j+1] = child_result.PV[j];
		                  result->PV[0] = moves[indiceMove];
		        }

			//part 2
		        if (ALPHA_BETA_PRUNING && child_score >= T->beta)
		          break;    

		        T->alpha = MAX(T->alpha, child_score);	
		}		
		
		if (TRANSPOSITION_TABLE)
		  tt_store(T, result);
		
        }else{
        //Esclave
        
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

		/* évalue récursivement les positions accessibles à partir d'ici */
		for (int i = 0; i < n_moves; i++) {
			tree_t child;
		        result_t child_result;
		        
		        play_move(T, moves[i], &child);
		        
		        evaluate(&child, &child_result,rang);
		                 
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
}


void decide(tree_t * T, result_t *result)
{
	for (int depth = 1;; depth++) {
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;

                printf("=====================================\n");
		evaluate(T, result, 0);

                printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
                print_pv(T, result);
                
                if (DEFINITIVE(result->score))
                  break;
	}
}

int main(int argc, char **argv)
{  
	/* initialisation MPI */
	int p, rang, tag = 10, idmaster = 0;
	int* tab;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	MPI_Comm_rank(MPI_COMM_WORLD, &rang);  
	MPI_Status status;
	
	/* Chronometrage */
  	double debut, fin;
  
	/* debut du chronometrage */
	debut = my_gettimeofday();
	
	/* MASTER*/
	if(rang==idmaster){
		
		tree_t root;
		result_t result;

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
		print_position(&root);
		decide(&root, &result);

		printf("\nDécision de la position: ");
		switch(result.score * (2*root.side - 1)) {
		case MAX_SCORE: printf("blanc gagne\n"); break;
		case CERTAIN_DRAW: printf("partie nulle\n"); break;
		case -MAX_SCORE: printf("noir gagne\n"); break;
		default: printf("BUG\n");
		}

		printf("Node searched: %llu\n", node_searched);
		
		if (TRANSPOSITION_TABLE)
		  free_tt();
		
		/* fin du chronometrage */
		fin = my_gettimeofday();
		fprintf( stderr, "#%d Temps total de calcul : %g sec\n",rang, fin - debut);
		fprintf( stdout, "#%d %g\n",rang, fin - debut);
		MPI_Finalize();
	}
	/* Esclave */
	else{
		//tant qui il y a du travail
		while(tag != TAG_END){
			printf("#%d coucou\n",rang);
			//receive job
			int Tr[535];
			MPI_Recv(Tr, 535, MPI_INT, idmaster, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			printf("#%d recu\n",rang);
			//effectue le job
			tag = status.MPI_TAG;
			if(tag == TAG_REQ){
				tree_t T;		
				//remplir T
				for(int i=0;i<128;i++){
					T.pieces[i]=(char)Tr[i];
				}
				for(int i=128;i<256;i++){
					T.colors[i]=(char)Tr[i];
				}T.side=Tr[257]; 	
				T.depth=Tr[258];
				T.height=Tr[259];
				T.alpha=Tr[260];
				T.beta=Tr[270];
				T.alpha_start=Tr[271];
				T.king[0]=Tr[272];
				T.king[1]=Tr[273];
				T.pawns[0]=Tr[274];
				T.pawns[1]=Tr[275];					
				for(int i=276;i<404;i++){
					T.attack[i]=(char)Tr[i];
				}
				T.suggested_move=Tr[404];
				T.suggested_move=(unsigned long long int)Tr[405];
				for(int i=406;i<534;i++){
					T.history[i]=(unsigned long long int)Tr[i];
				}
				result_t result;
				printf("travail fait de %d\n",rang);
				//job
				evaluate(&T, &result, rang);
				
				//envoie resultat result
				int R[MAX_DEPTH + 4];
				R[0]=result.score;
				R[1]=result.best_move;
				R[2]=result.pv_length;
				for(int i = 3;i<MAX_DEPTH;i++){
					R[i]=result.PV[i-3];
				}
				R[MAX_DEPTH + 3]=Tr[534];
				printf("travail fait de %d\n",rang);
				printf("#%d envoie score %d best_move %d pv_length%d result.PV %d %d %d\n",rang,R[0],R[1],R[2],R[3],R[4],R[MAX_DEPTH + 3]);
				MPI_Send(&R, MAX_DEPTH + 4, MPI_INT, idmaster, TAG_DATA, MPI_COMM_WORLD);
				printf("travail envoyé de %d\n",rang);
			}
		}
		/* fin du chronometrage */
		fin = my_gettimeofday();
		fprintf( stderr, "#%d Temps total de calcul : %g sec\n",rang, fin - debut);
		fprintf( stdout, "#%d %g\n",rang, fin - debut);
		MPI_Finalize();
	}
	
	return 0;
}
