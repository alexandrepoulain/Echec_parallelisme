#include "projet.h"

/* 2017-02-23 : version 1.0 */
/* Version MPI_Pur: maître-esclaves 
  Le maître envoi le premier niveau de l'arbre aux esclaves
  Problèmes: - le maître ne calcule pas
              - Une fois fini un esclave envoie son résultat et attend (il devrait aider)
*/
#define TAG_INIT 0
#define TAG_JETON_CALCUL 1
#define TAG_RESULT 2
#define TAG_DEMANDE 3
#define TAG_RESULT_DEMANDE 4
#define TAG_END 5

unsigned long long int node_searched = 0;

double my_gettimeofday(){
  struct timeval tmp_time;
  gettimeofday(&tmp_time, NULL);
  return tmp_time.tv_sec + (tmp_time.tv_usec * 1.0e-6L);
}

void evaluate(tree_t * T, result_t *result)
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

        /* évalue récursivement les positions accessibles à partir d'ici */
  for (int i = 0; i < n_moves; i++) {
    tree_t child;
    result_t child_result;

    play_move(T, moves[i], &child);

    evaluate(&child, &child_result);

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

/* Fonction evaluate root qui sera appeler seulement par le processus root */
void evaluate_root(tree_t * T, result_t *result, int tag, int NP, MPI_Status status, int rang)
{

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

  /* Plateau de jeu */
  const int nitems=14;
  int          blocklengths[14] = {128,128,1,1,1,1,1,1,2,2,128,1,1, MAX_DEPTH};
  MPI_Datatype types[14] = {MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT};
  MPI_Datatype mpi_tree_t;
  MPI_Aint     offsets[14];

  offsets[0] = offsetof(tree_t, pieces);
  offsets[1] = offsetof(tree_t, colors);
  offsets[2] = offsetof(tree_t, side);
  offsets[3] = offsetof(tree_t, depth);
  offsets[4] = offsetof(tree_t, height);
  offsets[5] = offsetof(tree_t, alpha);
  offsets[6] = offsetof(tree_t, beta);
  offsets[7] = offsetof(tree_t, alpha_start);
  offsets[8] = offsetof(tree_t, king);
  offsets[9] = offsetof(tree_t, pawns);
  offsets[10] = offsetof(tree_t, attack);
  offsets[11] = offsetof(tree_t, suggested_move);
  offsets[12] = offsetof(tree_t, hash);
  offsets[13] = offsetof(tree_t, history);

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_tree_t);
  MPI_Type_commit(&mpi_tree_t);
  MPI_Datatype datatype;
  int* count;

  node_searched++;
  
  move_t moves[MAX_MOVES];
  int n_moves;

  result->score = -MAX_SCORE - 1;
  result->pv_length = 0;

  if (test_draw_or_victory(T, result))
    return;

  if (TRANSPOSITION_TABLE && tt_lookup(T, result))     
    // la réponse est-elle déjà connue ? 
    return;

  compute_attack_squares(T);

  // profondeur max atteinte ? si oui, évaluation heuristique 
  if (T->depth == 0) {
    result->score = (2 * T->side - 1) * heuristic_evaluation(T);
    return;
  }

  n_moves = generate_legal_moves(T, &moves[0]);

  // absence de coups légaux : pat ou mat 
  if (n_moves == 0) {
    result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
    return;
  }

  if (ALPHA_BETA_PRUNING)
    sort_moves(T, n_moves, moves);

  // évalue récursivement les positions accessibles à partir d'ici 
  
  /*** IMPORTANT **/
  /* on les définis ici car ils vont être partgé entre les deux treads */
  tree_t child;
  result_t child_result;
  // On alloue la mémoire nécessaire à la demande 
  // Ici on fixe la taille à 3 moves
  move_t new_move;
  tree_t new_T, new_child;
  result_t new_child_result, new_result;
  // j correspond à l'indice où en est le processus de calcul
  int over = 0, new_over=0, termine_premiere_partie = 0, indice_calcul, indice_fin, nb_elem, nb_elem_demande;
  int fini = 1;
  // Initialisation du job tant qu'on peut on envoi un job à n processus
  //  Chaque processus doit commencer avec un job
  #pragma omp parallel sections
  {
     printf("On ouvre les sections paralleles pour le maitr\n");
    #pragma omp section
    {

      // On commence à envoyer à partir de l'indice 1 
      // ( l'indice 0 c'est le maitre qui s'en occupe)
      int reste = n_moves;
      int source;
      int nb_regions, indice_move, demandeur, index = 0;
      while (reste == n_moves){
        reste = n_moves%(NP-index);
        nb_regions = NP-index;
        nb_elem = n_moves/(NP-index);
      }
      // fixe l'indice de fin pour le processus 0
      indice_fin = nb_elem;

      for (int i = 1; i < nb_regions ; i++) {
        // SALE si on est arrivé au max du nombre de processus on arrête 
        // tant que le reste eest pas égal à 0 on rajoute un élément au message
        printf("#ROOT nb_regions = %d\n", nb_regions);
	if (reste!=0){
          move_t send_moves[nb_elem+1];
          reste--;
          // on remplit le tableau de moves à envoyer
          for (int j=0; j<nb_elem+1; j++){
            send_moves[j]=moves[nb_regions+j];
          }
          // on envoie au thread de comm du processus correspondant
          MPI_Send(T, 1, mpi_tree_t, i, TAG_INIT, MPI_COMM_WORLD);
          printf("#ROOT envoi à #%d\n", i);
          // Send au processus i du move
          //printf("#ROOT envoi du move %d à #%d\n",moves[i], i); 
          MPI_Send(&send_moves, nb_elem+1, MPI_INT, i, TAG_INIT, MPI_COMM_WORLD);
        }
        // le reste est égal à 0
        else{
          move_t send_moves[nb_elem];
          reste--;
          // on remplit le tableau de moves à envoyer
          for (int j=0; j<nb_elem; j++){
            send_moves[j]=moves[nb_regions+j];
          }
          // on envoie au thread de comm du processus correspondant
          MPI_Send(T, 1, mpi_tree_t, i, TAG_INIT, MPI_COMM_WORLD);
          //printf("#ROOT envoi à #%d\n", i);
          // Send au processus i du move
          //printf("#ROOT envoi du move %d à #%d\n",moves[i], i); 
          MPI_Send(&send_moves, nb_elem, MPI_INT, i, TAG_INIT, MPI_COMM_WORLD);
        }
	printf("#ROOT fin initialisation\n");	
      }
      /*** Première partie de l'initialisation terminée ***/
      /*** Attente que le processus de calcul est fini ***/
      
      while(fini)
      {
        // Un probe pour connaiître la nature du message à recevoir

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        tag = status.MPI_TAG; 
        // Receive d'un resultat de sous arbre
        if(tag == TAG_RESULT){
          MPI_Recv(&child_result, 1, mpi_result_t, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
          nb_regions--;
          int child_score = -child_result.score;
          if (child_score > result->score){
            result->score = child_score;
            // on recupere le move correspondant en utilisant le tableau indice
            result->best_move = child_result.best_move;
            result->pv_length = child_result.pv_length + 1;
            for(int j = 0; j < child_result.pv_length; j++)
              result->PV[j+1] = child_result.PV[j];
            result->PV[0] = child_result.best_move;
          }
          T->alpha = MAX(T->alpha, child_score);
          // Si toutes les régions ont répondu on arrête
          if(nb_regions == 0)
            fini = 0;
        }

        // Si on reçoit une demande de calcul en réponse à un jeton
        if(tag == TAG_DEMANDE){
          // On traite la demande
          demandeur = status.MPI_SOURCE;
          // On reçoit les moves
          MPI_Recv(&new_move,1,MPI_INT,status.MPI_SOURCE, tag, MPI_COMM_WORLD,&status); 
          // Récupération du plateau 
          MPI_Recv(&new_T,1,mpi_tree_t,status.MPI_SOURCE, tag, MPI_COMM_WORLD,&status);
          nb_elem_demande=1;
          // On signale au tread de calcul qu'il peux y aller
          over = 0;
        }

        // si le thread de calcul a fini on envoit le jeton de calcul
        if(over == 1){
          // c'est un anneau donc on envoit au process suivant: ici 1
          // On place dans l'élément envoyé le rang
          // Ainsi tout le monde pourra le distribuer en connaisant qui est l'envoyeur
          int rang_envoyeur = 0;
          MPI_Send(&rang_envoyeur,1, MPI_INT, 1,TAG_JETON_CALCUL, MPI_COMM_WORLD);
        }
        // Si le thread de calcul a fini sa demande
        if(new_over == 1){
          // On envoit le result au demandeur
          MPI_Send(&new_result, 1, mpi_result_t, demandeur, TAG_RESULT_DEMANDE, MPI_COMM_WORLD);
          // on va se placer en émission d'un jeton de calcul
          over = 1;
          new_over = 0;
        }

        // Si le thread reçoit un jeton de calcul
        // On va regarder où le processus de calcul en est et peut-être lui prendre une partie de son calcul
        if(tag == TAG_JETON_CALCUL){
          // on recupere l'envoyeur
          int envoyeur;
          MPI_Recv(&envoyeur,1,MPI_INT,status.MPI_SOURCE,tag,MPI_COMM_WORLD, &status);
          // on teste savoir si ce n'estas notre propre jeton de calcul
          if(envoyeur != rang){
            // On teste savoir si on est dans la première partie du calcul ou pas
            if(!termine_premiere_partie){
              // On teste si il y a besoin d'envoyer du calcul
              if(indice_calcul+2 < nb_elem){
                indice_fin--;
                MPI_Send(&moves[nb_elem-1],1,MPI_INT,status.MPI_SOURCE, TAG_DEMANDE, MPI_COMM_WORLD);
                MPI_Send(&T,1,mpi_tree_t, status.MPI_SOURCE, TAG_DEMANDE, MPI_COMM_WORLD);
                // On précise qu'une nouvelle région vient d'être crée
                nb_regions++;
              }
              // sinon on transmet le jeton de calcul
              else{
                MPI_Send(&envoyeur,1,MPI_INT,(rang+1), TAG_JETON_CALCUL, MPI_COMM_WORLD);
              }
            }
            // Sinon on est déjà dans un calcul demandé par un autre processus
            else{
              // Pour l'instant on fait rien mais bientot il pourra aider le calcul aussi ici
              // Du coup on transmet juste
              MPI_Send(&envoyeur,1,MPI_INT,(rang+1), TAG_JETON_CALCUL, MPI_COMM_WORLD);
            }
          }
        }
      }
    }
      
    /* chaques processus a du job à faire */
    /*** SECTION calcul première partie ***/
    #pragma omp section
    {
      // En gros sur chaque move on envoie evaluate 
      for(indice_calcul = 0; indice_calcul < nb_elem; indice_calcul++) {
        // Si on est arrivé au bout: en cas de raccourcissement
        if(indice_calcul >= indice_fin-1)
          break;
        play_move(T, moves[indice_calcul], &child);

        evaluate(&child, &child_result);

        int child_score = -child_result.score;

        if (child_score > result->score) {
         result->score = child_score;
         result->best_move = moves[indice_calcul];
         result->pv_length = child_result.pv_length + 1;
         for(int j = 0; j < child_result.pv_length; j++)
          result->PV[j+1] = child_result.PV[j];
         result->PV[0] = moves[indice_calcul];
        }

      }
      /*** Première partie du calcul fini ***/
      termine_premiere_partie = 1;
      over = 1; //signale au processus de calcul qu'il peut envoyer le jeton
      // Ici on se met en attente du process de communication
      // On recupère et on traite les demandes
      while(fini){
        // le process de calcul nous signale qu'on peux y aller
        if (over == 0 && new_over == 0){
          for (indice_calcul = 0; indice_calcul < nb_elem_demande; indice_calcul++) {

            play_move(&new_T, new_move, &new_child);

            evaluate(&new_child, &new_child_result);

          }
          // On a fini le calcul
          new_over = 1;
        }
      
    }
  }
}
}


void decide(tree_t * T, result_t *result, int tag, int NP, MPI_Status status, int rang)
{
	printf("#ROOT rentre dans decide\n");
	for (int depth = 1;; depth++) {
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;
    printf("=====================================\n");
    evaluate_root(T, result, tag, NP, status, rang);

    printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
    print_pv(T, result);

    if (DEFINITIVE(result->score))
      break;
  }
}

int main(int argc, char **argv)
{  
	

  /* Init MPI */
  int NP, rang, tag = 10;
  int provided;
  MPI_Init_thread(&argc,&argv, MPI_THREAD_MULTIPLE, &provided);
   
  if(provided < MPI_THREAD_MULTIPLE){
    printf("Error level provided less than required\n");
  }
  else{
    MPI_Comm_size(MPI_COMM_WORLD, &NP);
    // Le rang des processus
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    // le status
    MPI_Status status;
    
  /* On va creer toutes les structures pour l'envoi d'information par MPI
        Le premier pour le plateau de jeu
        et le deuxième pour la structure result
    */

    /* Plateau de jeu */
    const int nitems=14;
    int          blocklengths[14] = {128,128,1,1,1,1,1,1,2,2,128,1,1, MAX_DEPTH};
    MPI_Datatype types[14] = {MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_tree_t;
    MPI_Aint     offsets[14];

    offsets[0] = offsetof(tree_t, pieces);
    offsets[1] = offsetof(tree_t, colors);
    offsets[2] = offsetof(tree_t, side);
    offsets[3] = offsetof(tree_t, depth);
    offsets[4] = offsetof(tree_t, height);
    offsets[5] = offsetof(tree_t, alpha);
    offsets[6] = offsetof(tree_t, beta);
    offsets[7] = offsetof(tree_t, alpha_start);
    offsets[8] = offsetof(tree_t, king);
   offsets[9] = offsetof(tree_t, pawns);
    offsets[10] = offsetof(tree_t, attack);
    offsets[11] = offsetof(tree_t, suggested_move);
    offsets[12] = offsetof(tree_t, hash);
    offsets[13] = offsetof(tree_t, history);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_tree_t);
    MPI_Type_commit(&mpi_tree_t);

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

    


    /* Si je suis le processus 0 je rentre dans la fonction decide */
    if(rang == 0){
      int i; 
      double debut, fin;
      tree_t T;
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

      parse_FEN(argv[1], &T);
      //print_position(&T);
      debut = my_gettimeofday();
      // nowait pour pas qu'il attend l'autre
      // un thread sert 

      decide(&T, &result, tag, NP, status, rang);
      for(i=1; i<NP; i++){
        //printf("#ROOT envoi finalize %d\n", i);
        MPI_Send(&T, 1, mpi_tree_t, i, TAG_END, MPI_COMM_WORLD);
      }
      fin = my_gettimeofday();  
      fprintf( stderr, "Temps total de calcul : %g sec\n", 
       fin - debut);
      fprintf( stdout, "%g\n", fin - debut);
      printf("\nDécision de la position: ");
      switch(result.score * (2*T.side - 1)) {
        case MAX_SCORE: printf("blanc gagne\n"); break;
        case CERTAIN_DRAW: printf("partie nulle\n"); break;
        case -MAX_SCORE: printf("noir gagne\n"); break;
        default: printf("BUG\n");
    }
    printf("Node searched: %llu\n", node_searched);
      /* free everything */
      if (TRANSPOSITION_TABLE)
        free_tt();
      MPI_Type_free(&mpi_tree_t);
      MPI_Type_free(&mpi_result_t);
      MPI_Finalize();
    }
    /* sinon je suis dans un while tant qu'on me dit pas que c'est fini
      Au début je reçois un job et je l'execute 
      Dès que je l'ai fini je me signale au maître "Pret pour un nouveau job"
    */
    else{
      /*** PARTIE PARTAGE ***/
      tree_t root_proc; 
      move_t* move;
      int fini=1, source, go = 0, over, attente;
      int count, indice, nb_elem, indice_fin;
      result_t result;
      printf("#%d Au rapport\n", rang);
      #pragma omp parallel sections
      {
      	#pragma omp section
      	{	
        	int demandeur, envoyeur;
        	while(fini)
        	{
        		// Probe pour connaître la nature du receive
        		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_SOURCE, MPI_COMM_WORLD, &status);
            tag = status.MPI_TAG;
        		printf("#%d Je viens de recevoir un signal\n",rang);
  	        // Si c'est une initiation: on la prend (elle provient forcement de 0)
        		if(tag == TAG_INIT)
        		{
		          printf("#%d je reçois de ROOT \n",rang);
        			// Rceive tree
        			MPI_Recv(&root_proc, 1, mpi_tree_t, 0, TAG_INIT, MPI_COMM_WORLD, &status);
        			// Receive les moves
        			// Il faut connaître le nombre de moves à recevoir
        			MPI_Get_count(&status, MPI_INT, &count);
        			//Receive des moves
        			move = (move_t*)malloc(count*sizeof(move_t));
        			MPI_Recv(&move, count, MPI_INT, 0, TAG_INIT, MPI_COMM_WORLD, &status);
              // on lance le calcul 
              nb_elem = count;
              indice_fin =nb_elem;
              go = 1;
              // on stocke à qui on doit renvoyer
                demandeur = 0;
            }
            // Si le thread de calcul a fini on envoit le result au demandeur
            if(over == 1 && attente == 0)
            {
              // envoit du result
              MPI_Send(&result, 1, mpi_result_t, demandeur, tag, MPI_COMM_WORLD);
              // envoit du jeton de calcul
              int moi = rang; 
              MPI_Send(&moi, 1, MPI_INT, rang+1, TAG_JETON_CALCUL, MPI_COMM_WORLD);
              over = 0;
            }
            // Si on reçoit une demande
            if(tag == TAG_DEMANDE)
            {
              // on recupre le demandeur
              demandeur = status.MPI_SOURCE;
              // on reçoit le move
              MPI_Get_count(&status, MPI_INT, &count);
              move = (move_t*)malloc(count*sizeof(move_t));
              MPI_Recv(&move, count, MPI_INT, demandeur, TAG_DEMANDE, MPI_COMM_WORLD, &status);
              // recoit l'arbre
              MPI_Recv(&root_proc, 1, mpi_tree_t, demandeur, TAG_INIT, MPI_COMM_WORLD, &status);
              nb_elem = 1;
              go = 1;      
             }
              if(tag = TAG_RESULT){
                // On la reçoit et on la traite
                result_t new_child_result;
                MPI_Recv(&new_child_result, 1, mpi_result_t, envoyeur, tag, MPI_COMM_WORLD, &status);
                int child_score = -new_child_result.score;
                if (child_score > result.score) {
                 result.score = child_score;
                 result.best_move = new_child_result.best_move;
                 result.pv_length = new_child_result.pv_length + 1;
                 for(int j = 0; j < new_child_result.pv_length; j++)
                  result.PV[j+1] = new_child_result.PV[j];
                 result.PV[0] = new_child_result.best_move;
                }
                // On attend plus de résultat
                attente = 0;
              }
              //Si on reçoit un jeton de calcul
              if(tag == TAG_JETON_CALCUL){
                envoyeur = status.MPI_SOURCE;
                // on teste savoir si ce n'estas notre propre jeton de calcul
                if(envoyeur != rang){
                // On test où on en est
                  if(indice+2 < nb_elem){
                    indice_fin--;
                    MPI_Send(&move[nb_elem-1],1,MPI_INT,envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                    MPI_Send(&root_proc,1,mpi_tree_t, envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                    // On attend la réponse pour pas la perdre
                    attente = 1;
                  }
                }

              }
              // Si on a le signal de fin
              if(tag == TAG_END)
                fini = 0;
            }
          
          }
          /*** THREAD CALCUL ***/
          #pragma omp section
          {
            while(fini)
            {
              if(go == 1){
                tree_t child;
                result_t child_result;
                for(indice = 0; indice < nb_elem; indice++)
                {
                  if(indice >= indice_fin-1)
                    break;
                  play_move(&root_proc, move[indice], &child);

                  evaluate(&child, &child_result);

                  int child_score = -child_result.score;

                  if (child_score > result.score) {
                   result.score = child_score;
                   result.best_move = move[indice];
                   result.pv_length = child_result.pv_length + 1;
                   for(int j = 0; j < child_result.pv_length; j++)
                    result.PV[j+1] = child_result.PV[j];
                   result.PV[0] = move[indice];
                  }
                }
                over = 1;
                go = 0;
              }
            }
          }

      }
      MPI_Finalize();
    }
  }  

      
    
  return 0;
}