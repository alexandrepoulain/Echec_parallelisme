#include "projet.h"

/* 2017-02-23 : version 1.0 */
#define TAG_END 5

unsigned long long int node_searched = 0;

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
void evaluate_root(tree_t * T, result_t *result)
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
  /* Initialisation du job tant qu'on peut on envoi un job à n processus
    Chaque processus doit commencer avec un job */
  int job_sent = 0;
  for (int i = 0; i < n_moves ; i++) {
    /* SALE si on est arrivé au max du nombre de processus on arrête */
    if( i > np)
      break;

    /* Send au processus i du tableau T */
    MPI_Send(&T, 1, mpi_tree_t, i, tag, MPI_COMM_WORLD);
    /* Send au processus i du move */
    MPI_Send(&move[i], 1, MPI_INT, i, tag, MPI_COMM_WORLD);
    job_sent++;
  }
  /* variable qui va nous permettre de savoir quand arrêter d'envoyer des jobs */
  compt_sent = job_sent; 
  while(job_sent > 0){
    /* Receive d'un job par le processus k 
      On stocke la reception dans child_result
      On compare avec la valeur déjà présente*/
    result_t child_result;
    MPI_Recv(&child_result, 1, mpi_result_t, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
    job_sent--;
    int child_score = -child_result.score;
    if (child_score > result->score){
      result->score = child_score;
      result->best_move = moves[i];
      result->pv_length = child_result.pv_length + 1;
      for(int j = 0; j < child_result.pv_length; j++)
        result->PV[j+1] = child_result.PV[j];
      result->PV[0] = moves[i];
    }
    T->alpha = MAX(T->alpha, child_score);
    /* Si il reste du job à faire on en envoie au processus
        sinon on fait rien */
    if(compt_sent < n_moves){
      /* Send un job au processus qui vient d'envoyer son résultat */
      /* On renvoie T */
      MPI_Send(&T, 1, mpi_tree_t, status.MPI_SOURCE, tag, MPI_COMM_WORLD);
      /* on envoie un nouveau move */
      MPI_Send(&move[compt_sent], 1, MPI_INT, status.MPI_SOURCE, tag, MPI_COMM_WORLD);
      compt_sent++;
      job_sent++; 
    }
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
    evaluate(T, result);

    printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
    print_pv(T, result);

    if (DEFINITIVE(result->score))
      break;
  }
}

int main(int argc, char **argv)
{  
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
  print_position(&T);

  /* Init MPI */
  MPI_Init(&argc,&argv);

  /* On va creer toutes les structures pour l'envoi d'information par MPI
      Le premier pour le plateau de jeu
      et le deuxième pour la structure result
  */

  /* Plateau de jeu */
  const int nitems=14;
  int          blocklengths[14] = {128,128,1,1,1,1,1,1,2,2,128,1,1, MAX_DEPTH};
  MPI_Datatype types[2] = {MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT};
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
  const int nitems=4;
  int          blocklengths[4] = {1,1,1, MAX_DEPTH};
  MPI_Datatype types[2] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
  MPI_Datatype mpi_result_t;
  MPI_Aint     offsets[4];

  offsets[0] = offsetof(result_t, score);
  offsets[1] = offsetof(result_t, best_move);
  offsets[2] = offsetof(result_t, pv_length);
  offsets[3] = offsetof(result_t, PV);

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_result_t);
  MPI_Type_commit(&mpi_result_t);

  // nombre de processus
  int NP;
  MPI_Comm_size(MPI_COMM_WORLD, &NP);

  // Le rang des processus
  int rang;
  MPI_Comm_rank(MPI_COMM_WORLD, &rang);

  // le status
  MPI_Status status;
  // le tag 
  int tag = 10;

  /* Si je suis le processus 0 je rentre dans la fonction decide */
  if(rang == 0){
    decide(&T, &result);
    /* SALE envoie à tout les processus de la fin */
    for(i=0; i<NP; i++){
      MPI_Send(NULL, 1, mpi_tree_t, i, TAG_END, MPI_COMM_WORLD);
    }
  }
  /* sinon je suis dans un while tant qu'on me dit pas que c'est fini
    Au début je reçois un job et je l'execute 
    Dès que je l'ai fini je me signale au maître "Pret pour un nouveau job"
  */
  else{
    while(1){
      /* Début d'un evaluate root */
      /* receive T */
      tree_t root_proc; 
      move_t move;
      MPI_Recv(&root_proc, 1, mpi_tree_t, 0, tag, MPI_COMM_WORLD, &status);
      if(tag == TAG_END)
        break;
      /* receive le move */
      MPI_Recv(&move, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
      /* Do le premier job */
      tree_t child;
      result_t child_result;
      /* On play le move attribué et on rentre dans la fonction evaluate
      */
      play_move(root_proc, move, &child);

      evaluate(&child, &child_result);

      root_proc->alpha = MAX(root_proc->alpha, child_score);
      /* dès qu'on est arrivé là on a fini le job */
      /* on envoie le result */
      MPI_Send(&child_result, 1, mpi_result_t, 0, tag, MPI_COMM_WORLD);
    }
    MPI_Finalize();
  }
  if (rang == 0){
    printf("\nDécision de la position: ");
    switch(result.score * (2*root.side - 1)) {
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
  MPI_Free
  return 0;
}