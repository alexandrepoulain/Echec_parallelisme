Echec_parallelisme
===
    N8-IPA PARALLELISME
    FORTIN Pierre

Date: 07/05/2017

    ALOUI Driss
    POULAIN Alexandre
    
MAIN 4 Polytech Paris UPMC


Introduction
------------
Le but du projet est de paralléliser un programme séquentiel. Ce programme prend en entrée une position du jeu d'échec, qu'il tente de décider, c'est-à-dire renvoyer l'une de ces trois options :
- Le joueur blanc peut gagner, quoi que fasse le joueur noir
- Le joueur noir peut gagner, quoi que fasse le joueur banc
- La partie sera nulle

Utilisation
------------
Chaque répertoire correspond à une implementation. Le préfixe AB correspond aux implementations utilisant l'algorithme alpha beta

    Make
    Make host
    Make exec
    Make clean

La commande Make host permet de mettre à jour tout les hostfiles des sous dossiers avec celui situé à la racine.

Attention certaines bibliothèques peuvent être nécessaire, en plus des bibliothèques standards.

Quelques tests
------------

-------- 4k//4K//4P w --------


seq : 
25.54s


open (2c) : 
15.3s


mpi (4p) : 
13.91s


open task (2c) : 
15.2908s


open mpi (2c 5p) : 
9.93s


open task mpi (2c 5p) : 
10.0281s


-------- 4k//4K///4P w --------


seq : 
1614.8s


open (2c) : 
892.136s


mpi (4p) : 
608.785s


open task (2c) : 
826.466s


open mpi (2c 5p) : 
600.344s


open task mpi (2c 5p) : 
382.254s
