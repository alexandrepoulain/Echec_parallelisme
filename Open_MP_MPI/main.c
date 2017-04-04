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
#define TAG_RAPPORT 6

unsigned long long int node_searched = 0;

double my_gettimeofday(){
  struct timeval tmp_time;
  gettimeofday(&tmp_time, NULL);
  return tmp_time.tv_sec + (tmp_time.tv_usec * 1.0e-6L);
}

void free_chain(chained_t* root)
{
  printf("n_moves = %d \n", root->n_moves);
  if(root->n_moves != 0)
  {
    
    for(int i = 0; i < root->n_moves; i++)
    {
      //printf("Je plonge  \n");
      free_chain(root->chain[i]);
      free(root->chain[i]);
      //printf("Je sors \n");
      
    }
    free(root->moves);
  }
  
  
}

void evaluate(chained_t* root_chain)
{
  node_searched++;
  //printf("je construit\n");
  root_chain->moves = calloc(MAX_MOVES,sizeof(move_t));

  root_chain->result.score = -MAX_SCORE - 1;
  root_chain->result.pv_length = 0;

  if (test_draw_or_victory(&root_chain->plateau, &root_chain->result))
    return;

  if (TRANSPOSITION_TABLE && tt_lookup(&root_chain->plateau, &root_chain->result))     /* la réponse est-elle déjà connue ? */
    return;

  compute_attack_squares(&root_chain->plateau);

        /* profondeur max atteinte ? si oui, évaluation heuristique */
  if (root_chain->plateau.depth == 0) {
    root_chain->result.score = (2 * root_chain->plateau.side - 1) * heuristic_evaluation(&root_chain->plateau);
    return;
  }

  root_chain->n_moves = generate_legal_moves(&root_chain->plateau, &root_chain->moves[0]);
  root_chain->indice_fin = root_chain->n_moves;
  root_chain->chain = calloc(root_chain->n_moves,sizeof(chained_t*));

        /* absence de coups légaux : pat ou mat */
  if (root_chain->n_moves == 0) {
    root_chain->result.score = check(&root_chain->plateau) ? -MAX_SCORE : CERTAIN_DRAW;
    return;
  }
  /*
  if (ALPHA_BETA_PRUNING)
    sort_moves(T, n_moves, moves);
*/
        /* évalue récursivement les positions accessibles à partir d'ici */
  for ( root_chain->indice = 0; root_chain->indice < root_chain->indice_fin; root_chain->indice++) 
  {

    root_chain->chain[root_chain->indice] = calloc(1,sizeof(chained_t));
    play_move(&root_chain->plateau, root_chain->moves[root_chain->indice], &root_chain->chain[root_chain->indice]->plateau);

    evaluate(root_chain->chain[root_chain->indice]);

    int child_score = -root_chain->chain[root_chain->indice]->result.score;

    if (child_score > root_chain->result.score) 
    {
      root_chain->result.score = child_score;
      root_chain->result.best_move = root_chain->moves[root_chain->indice];
      root_chain->result.pv_length = root_chain->chain[root_chain->indice]->result.pv_length + 1;
      for(int j = 0; j < root_chain->chain[root_chain->indice]->result.pv_length; j++)
        root_chain->result.PV[j+1] = root_chain->chain[root_chain->indice]->result.PV[j];
      root_chain->result.PV[0] = root_chain->moves[root_chain->indice];
    }
    // free chain
    free(root_chain->chain[root_chain->indice]);
 
  if (ALPHA_BETA_PRUNING && child_score >= root_chain->plateau.beta)
    break;    

  root_chain->plateau.alpha = MAX(root_chain->plateau.alpha, child_score);


  if (TRANSPOSITION_TABLE)
    tt_store(&root_chain->plateau, &root_chain->result);
  
  }
  
  while(root_chain->indice_fin != root_chain->n_moves){
    ;
  }
  free(root_chain->chain);
  free(root_chain->moves);
  
  //printf("Je détruit\n");
  //free_chain(root_chain);
}

// Cette fonction va parcourir l'arbre de calcul et retourner le calcul possible (son adresse)
chained_t* cherche_calcul(chained_t* node)
{
  int depth = 0;
  while(node->indice == node->indice_fin-1 && node->fini != 1){
    if(depth >= 5)
      return NULL; 
    node = node->chain[node->indice_fin-1];
    depth++;
  }
  printf("here\n");
  if(node->indice < node->indice_fin-1){
    
    node->indice_fin--;
    printf("profondeur = %d\n", depth);
    return node;
  }
  else{
    return NULL;
  }
}




/* Fonction evaluate root qui sera appeler seulement par le processus root */
void evaluate_root(chained_t* root_chain, int tag, int NP, MPI_Status status, int rang, MPI_Datatype mpi_tree_t, MPI_Datatype mpi_result_t)
{
  int* count;

  node_searched++;
  
  chained_t new_root_chain;

  root_chain->moves = calloc(MAX_MOVES,sizeof(move_t));
  printf("#ROOT Test\n");
  root_chain->result.score = -MAX_SCORE - 1;
  root_chain->result.pv_length = 0;

  if (test_draw_or_victory(&root_chain->plateau, &root_chain->result))
    return;

  if (TRANSPOSITION_TABLE && tt_lookup(&root_chain->plateau, &root_chain->result))     /* la réponse est-elle déjà connue ? */
    return;

  compute_attack_squares(&root_chain->plateau);

  /* profondeur max atteinte ? si oui, évaluation heuristique */
  if (root_chain->plateau.depth == 0) {
    root_chain->result.score = (2 * root_chain->plateau.side - 1) * heuristic_evaluation(&root_chain->plateau);
    return;
  }
  #pragma omp critical
  {
    root_chain->n_moves = generate_legal_moves(&root_chain->plateau, &root_chain->moves[0]);
    root_chain->chain = calloc(root_chain->n_moves,sizeof(chained_t*));
  }

        /* absence de coups légaux : pat ou mat */
  if (root_chain->n_moves == 0) {
    root_chain->result.score = check(&root_chain->plateau) ? -MAX_SCORE : CERTAIN_DRAW;
    return;
  }
  // évalue récursivement les positions accessibles à partir d'ici 
  
  /*** IMPORTANT **/
  /* on les définis ici car ils vont être partgé entre les deux treads */
  // On alloue la mémoire nécessaire à la demande 
  // Ici on fixe la taille à 3 moves
  int over = 0, new_over=0, termine_premiere_partie = 0, nb_elem;
  int fini = 1, go=0;
  // Initialisation du job tant qu'on peut on envoi un job à n processus
  //  Chaque processus doit commencer avec un job
  #pragma omp parallel sections
  {
     printf("On ouvre les sections paralleles pour le maitr\n");
    #pragma omp section
    {
      // On définit un tableau d'adresse: quand le processus enverra une partie du calcul à un autre processus
      // A l'aide de ca tableau il connaîtra l'adresse du noeud où vient le calcul
      chained_t* adresse[NP];
      

      // On commence à envoyer à partir de l'indice 1 
      // ( l'indice 0 c'est le maitre qui s'en occupe)
      printf("#ROOT il y a %d moves pour %d processus et ils sont les suivant:\n", root_chain->n_moves, NP);
      for(int r = 0; r < root_chain->n_moves; r++)
        printf("#ROOT: move[%d] = %d \n", r, root_chain->moves[r]);
      int reste = root_chain->n_moves;
      int source;
      int nb_regions, indice_move, demandeur, index = 0;
      while(reste == root_chain->n_moves){
        reste = (root_chain->n_moves)%(NP-index);
        nb_regions = NP-index;
        nb_elem = (root_chain->n_moves)/(NP-index);
        index++;
      }
      root_chain->n_moves = nb_elem;
      root_chain->indice_fin = root_chain->n_moves;
      printf("#ROOT reste = %d\n", reste);
      // Processus 0 peut commencer
      #pragma omp critical
      go = 1;
      nb_regions --; 
      int temp_reste = reste;
      int nb_reg = 1;
      for (int i = 0; i < nb_regions ; i++) 
      {
        // SALE si on est arrivé au max du nombre de processus on arrête 
        // tant que le reste eest pas égal à 0 on rajoute un élément au message
        printf("#ROOT indice = %i\n", i);
        if(reste != 0){
          move_t send_moves[nb_elem+1];
          // on remplit le tableau de moves à envoyer
          for (int j=0; j<nb_elem+1; j++)
          {
            send_moves[j]=root_chain->moves[(nb_elem+1)*i+nb_elem+j];
          }
          // on envoie au thread de comm du processus correspondant
          MPI_Send(&root_chain->plateau, 1, mpi_tree_t, i+1, TAG_INIT, MPI_COMM_WORLD);
          printf("#ROOT envoi à #%d de %d moves\n", i+1, nb_elem+1);
          // Send au processus i du move
          //printf("#ROOT envoi du move %d à #%d\n",moves[i], i); 
          MPI_Send(&send_moves[0], nb_elem+1, MPI_INT, i+1, TAG_INIT, MPI_COMM_WORLD);
          reste--;
        }
        else{
          move_t send_moves[nb_elem];
          // on remplit le tableau de moves à envoyer
          for (int j=0; j<nb_elem; j++)
          {
            send_moves[j]=root_chain->moves[(nb_elem+1)*temp_reste+nb_reg*nb_elem+j];
          }
          // on envoie au thread de comm du processus correspondant
          MPI_Send(&root_chain->plateau, 1, mpi_tree_t, i+1, TAG_INIT, MPI_COMM_WORLD);
          printf("#ROOT envoi à #%d de %d moves\n", i+1, nb_elem);
          // Send au processus i du move
          //printf("#ROOT envoi du move %d à #%d\n",moves[i], i); 
          MPI_Send(&send_moves[0], nb_elem, MPI_INT, i+1, TAG_INIT, MPI_COMM_WORLD);
          nb_reg++;
        }
      }
      // On retire la région dont le maître s'occupe
      
      printf("#ROOT fin initialisation\n"); 
      /*** Première partie de l'initialisation terminée ***/
      /*** Attente que le processus de calcul est fini ***/
      int flag;
      int temp_fin = 1, new_fini = 1;
      int* test_fin = calloc(NP, sizeof(int));
      test_fin[0]=1;
      while(temp_fin)
      {
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        // Un probe pour connaiître la nature du message à recevoir
        //si le thread de calcul a fini on envoit le jeton de calcul
        if(over == 1){
          // c'est un anneau donc on envoit au process suivant: ici 1
          // On place dans l'élément envoyé le rang
          // Ainsi tout le monde pourra le distribuer en connaisant qui est l'envoyeur
          printf("#ROOT j'envoie mon jeton de calcul\n");
          int rang_envoyeur = 0;
          MPI_Send(&rang_envoyeur,1, MPI_INT, 1,TAG_JETON_CALCUL, MPI_COMM_WORLD);
          #pragma omp critical
          over = 0;
        }
        // Si le thread de calcul a fini sa demande
        if(new_over == 1){
          printf("#ROOT J'ai le résultat pour %d\n", demandeur);
          // On envoit le result au demandeur
          MPI_Send(&new_root_chain.result, 1, mpi_result_t, demandeur, TAG_RESULT_DEMANDE, MPI_COMM_WORLD);
          // on va se placer en émission d'un jeton de calcul
          #pragma omp critical
          over = 1;
          #pragma omp critical
          new_over = 0;
          printf("#ROOT J'ai envoyé le résultat pour %d\n", demandeur);
        }
        if (flag)
        {
          printf("#ROOT je reçois un message de %d, requête de type %d, avec le flag %d\n", status.MPI_SOURCE, status.MPI_TAG, flag);
          tag = status.MPI_TAG; 
          // Receive d'un resultat de sous arbre
          if(tag == TAG_RESULT)
          {
            result_t child_result;
            MPI_Recv(&child_result, 1, mpi_result_t, status.MPI_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);
            nb_regions--;
            #pragma omp critical
            {
              int child_score = child_result.score;
              if (child_score > root_chain->result.score){
                printf("#ROOT meilleur result par %d\n", status.MPI_SOURCE);
                root_chain->result.score = child_score;
                // on recupere le move correspondant en utilisant le tableau indice
                root_chain->result.best_move = child_result.best_move;
                root_chain->result.pv_length = child_result.pv_length + 1;
                for(int j = 0; j < child_result.pv_length; j++)
                  root_chain->result.PV[j+1] = child_result.PV[j];
                root_chain->result.PV[0] = child_result.best_move;
              }
              root_chain->plateau.alpha = MAX(root_chain->plateau.alpha, child_score);
            }
            printf("#ROOT receive score = %d \n", child_result.score);
            printf("#ROOT main_score = %d \n", root_chain->result.score);
              // Si toutes les régions ont répondu on arrête
            if(nb_regions == 0)
            {
              #pragma omp critical
              fini = 0;
            }
            printf("#ROOT bien reçu et traité %d --- il reste %d régions à venir\n", status.MPI_SOURCE, nb_regions);
          }
          // On reçoit un signal de rapport
          if(tag==TAG_RAPPORT){
            printf("#ROOT je reçoit un rapport de %d\n", status.MPI_SOURCE);
            int a;
            MPI_Recv(&a, 1, MPI_INT, status.MPI_SOURCE, TAG_RAPPORT, MPI_COMM_WORLD, &status);
            test_fin[status.MPI_SOURCE]=1;

          }


          // Si on reçoit une demande de calcul en réponse à un jeton
          if(tag == TAG_DEMANDE){

            // On traite la demande
            // on alloue la mémoire
            demandeur = status.MPI_SOURCE;
            printf("#ROOT je reçoie une demande de %d", demandeur);
            // Récupération du plateau 
            MPI_Recv(&new_root_chain.plateau,1,mpi_tree_t,status.MPI_SOURCE, TAG_DEMANDE, MPI_COMM_WORLD,&status);
            // On reçoit les moves
            int new_count;
            MPI_Probe(status.MPI_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_INT, &new_count);
            MPI_Recv(&new_root_chain.moves[0],new_count,MPI_INT,status.MPI_SOURCE, TAG_DEMANDE, MPI_COMM_WORLD,&status); 
            new_root_chain.n_moves=1;
            // On signale au tread de calcul qu'il peux y aller
            #pragma omp critical
            go = 1;
            printf("#ROOT j'ai lancé le calcul\n");
          }
          
          
          // Si le thread reçoit un jeton de calcul
          // On va regarder où le processus de calcul en est et peut-être lui prendre une partie de son calcul
          if(tag == TAG_JETON_CALCUL){
            // on recupere l'envoyeur
            int envoyeur;
            MPI_Recv(&envoyeur,1,MPI_INT,status.MPI_SOURCE,TAG_JETON_CALCUL,MPI_COMM_WORLD, &status);
            printf("#ROOT J'ai reçu le jeton de calcul de %d \n", envoyeur);
            // on teste savoir si ce n'estas notre propre jeton de calcul
            if(envoyeur != rang){
              // On teste savoir si on est dans la première partie du calcul ou pas
              if(!termine_premiere_partie){
                //On va chercher du calcul
                chained_t* parcours = cherche_calcul(root_chain);
                if(parcours != NULL && root_chain->plateau.depth > 4)
                {
                  printf("#ROOT j'envoie du calcul à %d\n", envoyeur);
                  adresse[envoyeur]=parcours;
                  // c'est qu'on a trouvé du calcul à une profondeur donnée
                  #pragma omp critical
                  {
                    parcours->indice_fin--;
                    MPI_Send(&parcours->plateau,1,mpi_tree_t, envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                    MPI_Send(&parcours->moves[parcours->indice_fin],1,MPI_INT,envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                    // On précise qu'une nouvelle région vient d'être crée
                    nb_regions++;
                  }
                }
                // sinon on transmet le jeton au suivant 
                else
                {
                  printf("#ROOT pas de calcul je transmet\n");
                  MPI_Send(&envoyeur,1,MPI_INT,1, TAG_JETON_CALCUL, MPI_COMM_WORLD);
                }
              }
              // Sinon on est déjà dans un calcul demandé par un autre processus
              else
              {
                // ici on va chercher recursivement
                // on définit l'addresse du noeud courant
                if(new_root_chain.plateau.depth > 4){
                  chained_t* parcours = cherche_calcul(&new_root_chain);
                  if(parcours != NULL){
                    printf("#ROOT j'envoie du calcul à %d\n", envoyeur);
                    // on va envoyer au demandeur le calcul correspondant à cette adresse
                    adresse[envoyeur] = parcours;
                    // Maintenant on peut envoyer
                    #pragma omp critical
                    MPI_Send(&parcours->plateau,1,mpi_tree_t, envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                    MPI_Send(&parcours->moves[parcours->indice_fin],1,MPI_INT, envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                  }
                  else{
                    // Du coup on transmet juste
                    printf("#ROOT pas de calcul je transmet\n");
                    MPI_Send(&envoyeur,1,MPI_INT,1, TAG_JETON_CALCUL, MPI_COMM_WORLD);
                  }
                }
                else{
                  // Du coup on transmet juste
                  printf("#ROOT pas de calcul je transmet\n");
                  MPI_Send(&envoyeur,1,MPI_INT,1, TAG_JETON_CALCUL, MPI_COMM_WORLD);
                }
              }
                
            }
            
          }
        }
        if(fini == 0){
          int test = 0;
          for(int g = 0; g < NP; g++){
            if(test_fin[g] != 1)
              test = 1;
          }
          if(test==0)
            #pragma omp critical
            temp_fin = 0;
        }
      }
      printf("#ROOT fini = %d\n", temp_fin);
    }
      
    /* chaques processus a du job à faire */
    /*** SECTION calcul première partie ***/
    #pragma omp section
    {
      int temp_go, temp_fin = 1;
      while(temp_fin)
      {
        #pragma omp critical
        {
          temp_fin = fini;
          temp_go = go;
        }
        
        if(temp_go)
        {
          printf("#ROOT je commence le calcul\n");
          // En gros sur chaque move on envoie evaluate 
          
            for(root_chain->indice = 0; root_chain->indice < root_chain->indice_fin; root_chain->indice++) 
            {
              root_chain->chain[root_chain->indice] = calloc(1,sizeof(chained_t));
              root_chain->chain[root_chain->indice]->indice = 0;
              root_chain->chain[root_chain->indice]->fini = 0;
              printf("#ROOT je calcul %d \n", root_chain->moves[root_chain->indice]);
              play_move(&root_chain->plateau, root_chain->moves[root_chain->indice], &root_chain->chain[root_chain->indice]->plateau);
              printf("#ROOT j'ai joué le move calcul %d \n", root_chain->moves[root_chain->indice]);
              evaluate(root_chain->chain[root_chain->indice]);
              printf("#ROOT je sors de evaluate pour le move %d \n", root_chain->moves[root_chain->indice]);
              int child_score = -root_chain->chain[root_chain->indice]->result.score;
              
                if (child_score > root_chain->result.score) 
                {
                 root_chain->result.score = child_score;
                 root_chain->result.best_move = root_chain->moves[root_chain->indice];
                 root_chain->result.pv_length = root_chain->chain[root_chain->indice]->result.pv_length + 1;
                 for(int j = 0; j < root_chain->chain[root_chain->indice]->result.pv_length; j++)
                  root_chain->result.PV[j+1] = root_chain->chain[root_chain->indice]->result.PV[j];
                 root_chain->result.PV[0] = root_chain->moves[root_chain->indice];
                }
                free(root_chain->chain[root_chain->indice]);
            }
            while(root_chain->indice_fin != root_chain->n_moves){
              ;
            }
            // le calcul est actuellement fini
            root_chain->fini = 1;
            free(root_chain->chain);
            free(root_chain->moves);
          
          #pragma omp critical 
          go = 0;
          //free(root_chain->moves);
          printf("#ROOT j'ai fini la première partie du calcul\n");
          /*** Première partie du calcul fini ***/
          #pragma omp critical
          {
            termine_premiere_partie = 1;
            over = 1; //signale au processus de calcul qu'il peut envoyer le jeton
            temp_fin = 1;
          }
          // Ici on se met en attente du process de communication
          // On recupère et on traite les demandes
          
          while(temp_fin)
          {
            #pragma omp critical
            temp_go = go;
            // le process de calcul nous signale qu'on peux y aller
            if (temp_go)
            {
              printf("#ROOT je rentre dans le nouveau calcul \n");
              for (new_root_chain.indice = 0; new_root_chain.indice < new_root_chain.n_moves; new_root_chain.indice++) 
              {
                new_root_chain.chain[new_root_chain.indice] = calloc(1,sizeof(chained_t));
                new_root_chain.chain[new_root_chain.indice]->indice = 0;
                new_root_chain.chain[new_root_chain.indice]->fini = 0;
                play_move(&new_root_chain.plateau, new_root_chain.moves[new_root_chain.indice], &new_root_chain.chain[new_root_chain.indice]->plateau);
                evaluate(new_root_chain.chain[new_root_chain.indice]);
                int child_score = -new_root_chain.chain[new_root_chain.indice]->result.score;
                if (child_score > new_root_chain.result.score){
                  new_root_chain.result.score = child_score;
                  // on recupere le move correspondant en utilisant le tableau indice
                  new_root_chain.result.best_move = new_root_chain.chain[new_root_chain.indice]->result.best_move;
                  new_root_chain.result.pv_length = new_root_chain.chain[new_root_chain.indice]->result.pv_length + 1;
                  for(int j = 0; j < new_root_chain.chain[new_root_chain.indice]->result.pv_length; j++)
                    new_root_chain.result.PV[j+1] = new_root_chain.chain[new_root_chain.indice]->result.PV[j];
                  new_root_chain.result.PV[0] = new_root_chain.chain[new_root_chain.indice]->result.best_move;
                }
                new_root_chain.plateau.alpha = MAX(new_root_chain.plateau.alpha, child_score);

              }
              int test = 1;
              while(new_root_chain.indice_fin != new_root_chain.n_moves){
                if(test == 1){
                  printf("#ROOT j'attends le reste du calcul\n");
                  test = 0;
                }
                
              }
              printf("#ROOT fini d'attendre\n");
              new_root_chain.chain[new_root_chain.indice]->fini = 1;
              // On a fini le calcul
              #pragma omp critical
              {
                new_over = 1;
                go = 0;
              }
            }
            #pragma omp critical
            {
              temp_fin = fini;
            }
            
          }
          
        
        }
      }
    }
      
  }


  printf("#ROOT je viens sort de evaluate_root et retourne dans decide \n");
  //free(root_chain.chain);
  //free_chain(&root_chain);
  //free_chain(&root_chain);
}


  



void decide(chained_t* root_chain, int tag, int NP, MPI_Status status, int rang, MPI_Datatype mpi_tree_t, MPI_Datatype mpi_result_t)
{
	printf("#ROOT rentre dans decide\n");
	for (int depth = 1;; depth++) {
		root_chain->plateau.depth = depth;
		root_chain->plateau.height = 0;
		root_chain->plateau.alpha_start = root_chain->plateau.alpha = -MAX_SCORE - 1;
		root_chain->plateau.beta = MAX_SCORE + 1;
    printf("=====================================\n");
    evaluate_root(root_chain, tag, NP, status, rang, mpi_tree_t, mpi_result_t);

    printf("depth: %d / score: %.2f / best_move : ", root_chain->plateau.depth, 0.01 * root_chain->result.score);
    print_pv(&root_chain->plateau, &root_chain->result);

    if (DEFINITIVE(root_chain->result.score)){
      printf("#ROOT sort de decide\n");
      break;
    }
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
      chained_t root_chain;
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

      parse_FEN(argv[1], &root_chain.plateau);
      //print_position(&T);
      debut = my_gettimeofday();
      // nowait pour pas qu'il attend l'autre
      // un thread sert 

      decide(&root_chain, tag, NP, status, rang, mpi_tree_t, mpi_result_t);
      for(i=1; i<NP; i++){
        //printf("#ROOT envoi finalize %d\n", i);
        MPI_Send(&root_chain.plateau, 1, mpi_tree_t, i, TAG_END, MPI_COMM_WORLD);
      }
      fin = my_gettimeofday();  
      fprintf( stderr, "Temps total de calcul : %g sec\n", 
       fin - debut);
      fprintf( stdout, "%g\n", fin - debut);
      printf("\nDécision de la position: ");
      switch(root_chain.result.score * (2*root_chain.plateau.side - 1)) {
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
      chained_t root_chain;
      int fini=1, source, go = 0, over, attente=0;
      int count, nb_elem;
      printf("#%d Au rapport\n", rang);
      #pragma omp parallel sections
      {
      	#pragma omp section
      	{	
          chained_t* adresse[NP];
        	int demandeur, envoyeur, ne_pas_rentrer = 0, flag;
        	while(fini)
        	{
            // Si le thread de calcul a fini on envoit le result au demandeur
            if(over == 1 && attente == 0)
            {
              printf("#%d essaye d'envoyer un result à %d \n", rang, demandeur);
              // envoit du result
              #pragma omp critical
              MPI_Send(&root_chain.result, 1, mpi_result_t, demandeur, TAG_RESULT, MPI_COMM_WORLD);
              // On détruit ici
              //#pragma omp critical
              //free_chain(&root_chain);
              
              // envoit du jeton de calcul
              int moi = rang; 
              MPI_Send(&moi, 1, MPI_INT, (rang+1)%NP, TAG_JETON_CALCUL, MPI_COMM_WORLD);
              
              #pragma omp critical
              over = 0;
            }
        		// Probe pour connaître la nature du receive (NON BLOQUANT)
            
        		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
            if(flag)
            {
              //printf("#%d flag = %d\n",rang,flag);
              tag = status.MPI_TAG;
              envoyeur = status.MPI_SOURCE;
          		//printf("#%d Je viens de recevoir un signal\n",rang);
    	        // Si c'est une initiation: on la prend (elle provient forcement de 0)
          		if(tag == TAG_INIT)
          		{
  		          printf("#%d je reçois de ROOT \n",rang);
          			// Rceive tree
          			MPI_Recv(&root_chain.plateau, 1, mpi_tree_t,0 , TAG_INIT, MPI_COMM_WORLD, &status);
                printf("#%d j'ai reçu l'arbre de ROOT \n",rang);
          			// Receive les moves
          			// Il faut connaître le nombre de moves à recevoir
                MPI_Probe(status.MPI_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
          			MPI_Get_count(&status, MPI_INT, &count);
          			//Receive des moves
                printf("#%d Je reçois %d moves \n",rang, count);
                #pragma omp critical
                {
            			root_chain.moves = calloc(count,sizeof(move_t));
            			MPI_Recv(&root_chain.moves[0], count, MPI_INT, 0, TAG_INIT, MPI_COMM_WORLD, &status);
                  // construction de la root_chain
                  root_chain.n_moves = count;
                  root_chain.indice_fin = count;
                  printf("#%d j'ai reçu les moves de ROOT \n",rang);
                  go = 1;
                  // on stocke à qui on doit renvoyer
                  demandeur = 0;
                }
              }
              
              // Si on reçoit une demande
              if(tag == TAG_DEMANDE)
              {
                printf("#%d reçoit une demande de calcul de %d  \n", rang, envoyeur);
                // on recupre le demandeur
                demandeur = status.MPI_SOURCE;
                // recoit l'arbre
                MPI_Recv(&root_chain.plateau, 1, mpi_tree_t, demandeur, TAG_INIT, MPI_COMM_WORLD, &status);
                // on reçoit le move
                MPI_Probe(status.MPI_SOURCE, TAG_INIT, MPI_COMM_WORLD, &status);
                MPI_Get_count(&status, MPI_INT, &count);
                root_chain.moves = calloc(count,sizeof(move_t));
                MPI_Recv(&root_chain.moves[0], count, MPI_INT, demandeur, TAG_DEMANDE, MPI_COMM_WORLD, &status);
                root_chain.n_moves = count;
                go = 1;      
               }
               
              
              if(tag == TAG_RESULT)
              {
                printf("#%d reçoit un resultat de %d \n", rang, envoyeur);
                // On la reçoit et on la traite
                result_t new_child_result;
                MPI_Recv(&new_child_result, 1, mpi_result_t, envoyeur, tag, MPI_COMM_WORLD, &status);
                int child_score = -new_child_result.score;
                if (child_score > adresse[envoyeur]->result.score){
                 adresse[envoyeur]->result.score = child_score;
                 adresse[envoyeur]->result.best_move = new_child_result.best_move;
                 adresse[envoyeur]->result.pv_length = new_child_result.pv_length + 1;
                 for(int j = 0; j < new_child_result.pv_length; j++)
                  adresse[envoyeur]->result.PV[j+1] = new_child_result.PV[j];
                 adresse[envoyeur]->result.PV[0] = new_child_result.best_move;
                }
                // On augmente l'indice de fin
                adresse[envoyeur]->indice_fin++;
              }
              
              //Si on reçoit un jeton de calcul
              if(tag == TAG_JETON_CALCUL)
              {
                MPI_Recv(&envoyeur,1,MPI_INT,status.MPI_SOURCE,TAG_JETON_CALCUL,MPI_COMM_WORLD, &status);
                if(envoyeur != rang){
                  printf("#%d reçoit le jeton de calcul de %d \n", rang, envoyeur);
                  // ici on va chercher recursivement
                  // on définit l'addresse du noeud courant
                  #pragma omp critical
                  {
                    if(root_chain.plateau.depth > 4)
                    {
                      chained_t* parcours = cherche_calcul(&root_chain);

                      if(parcours != NULL){
                        // on va envoyer au demandeur le calcul correspondant à cette adresse
                        adresse[envoyeur] = parcours;
                        // Maintenant on peut envoyer
                        
                        MPI_Send(&parcours->plateau,1,mpi_tree_t, envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                        MPI_Send(&parcours->moves[parcours->indice_fin],1,MPI_INT,envoyeur, TAG_DEMANDE, MPI_COMM_WORLD);
                        printf("#%d envoie du calcul à %d à une profondeur d'arbre %d\n",rang, envoyeur, root_chain.plateau.depth);
                      }
                      else
                      {
                        printf("#%d transmet le jeton de calcul de %d\n",rang, envoyeur);
                        // Du coup on transmet juste
                        MPI_Send(&envoyeur,1,MPI_INT,(rang+1)%NP, TAG_JETON_CALCUL, MPI_COMM_WORLD);
                      }
                    }
                    else
                    {
                      printf("#%d transmet le jeton de calcul de %d\n",rang, envoyeur);
                      // Du coup on transmet juste
                      MPI_Send(&envoyeur,1,MPI_INT,(rang+1)%NP, TAG_JETON_CALCUL, MPI_COMM_WORLD);
                    }
                  }
                }
                // Sinon on récupère notre propre jeton de calcul et on informe le maître qu'on est au rapport
                else{
                  printf("#%d j'informe le maître que je suis au rapport\n", rang);
                  int a = 0;
                  MPI_Send(&a, 1, MPI_INT, 0, TAG_RAPPORT, MPI_COMM_WORLD);
                }
              }
              
              // Si on a le signal de fin
              if(tag == TAG_END)
                fini = 0;
            }
          }
        }
        /*** THREAD CALCUL ***/
        #pragma omp section
        {
          int temp_go, temp_nb_elem;
          while(fini)
          {
            #pragma omp critical
            temp_go = go;
            if(temp_go == 1){
              #pragma omp critical
              {
                temp_nb_elem = nb_elem;
                root_chain.chain = calloc(root_chain.n_moves, sizeof(chained_t*));
              }
              printf("#%d commence le calcul sur %d moves \n", rang, temp_nb_elem);
              #pragma omp critical
              {
                for(root_chain.indice = 0; root_chain.indice < root_chain.indice_fin; root_chain.indice++)
                {
                  root_chain.chain[root_chain.indice] = calloc(1,sizeof(chained_t));
                  printf("#%d test calcul avant evaluate %d\n", rang, root_chain.moves[root_chain.indice]);
                  play_move(&root_chain.plateau, root_chain.moves[root_chain.indice], &(root_chain.chain[root_chain.indice]->plateau));
                  printf("#%d entre dans evaluate pour le move %d\n", rang, root_chain.moves[root_chain.indice]);
                  evaluate(root_chain.chain[root_chain.indice]);
                  printf("#%d fini evaluate pour le move %d\n", rang, root_chain.moves[root_chain.indice]);
                  
                  int child_score = -root_chain.chain[root_chain.indice]->result.score;

                  if (child_score > root_chain.chain[root_chain.indice]->result.score) {
                   root_chain.result.score = child_score;
                   root_chain.result.best_move = root_chain.moves[root_chain.indice];
                   root_chain.result.pv_length = root_chain.chain[root_chain.indice]->result.pv_length + 1;
                   for(int j = 0; j < root_chain.chain[root_chain.indice]->result.pv_length; j++)
                    root_chain.result.PV[j+1] = root_chain.chain[root_chain.indice]->result.PV[j];
                   root_chain.result.PV[0] = root_chain.moves[root_chain.indice];

                  }
                  free(root_chain.chain[root_chain.indice]);
                }
                free(root_chain.chain);
                free(root_chain.moves);
                // Si il y a du calcul à recuperer
                while(root_chain.indice_fin != root_chain.n_moves){
                  ;
                }
                printf("#%d Fini le calcul\n", rang);
              
                over = 1;
                go = 0;
              }
             
            }
          }
        }

      }
      printf("#%d je suis sorti et finalized \n", rang);
      MPI_Finalize();
    }
    
  }
      
    
  return 0;
}
