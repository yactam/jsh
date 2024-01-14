# Description de l'éxécution générale d'une commande

Au démarrage du shell il initialise une liste pour stocker les jobs à surveiller et il ignore les signaux SIGINT, SIGTERM, SIGTTIN, SIGQUIT, SIGTTOU et SIGTSTP, puis il rentre dans une boucle ou il affiche le prompt et il attend des commandes de la part de l'utilisateur, il lit la commande, il la coupe en utilisant l'espace comme séparateur pour avoir les différents tokens de la commande et il appel la fonction run_command qui filtre selon le type de la commande la fonction à appeler, si c'est une commande interne alors on appelle la fonction run_intern_command qui lance l'une des commandes dans le fichier intern_commands.c, si c'est une commande extern alors on appelle la fonction run_extern_command qui crée un job pour la commande, fait un fork ou le fils remis le gestionnaire des signaux par défaut et execute la commande spécifié et le shell démarre un nouveau groupe pour ce job et s'il est en avant-plan lui donne accés au terminal et fait une attente bloquante sur les processus de ce job (qui sont dans le même groupe) et met à jour son état sinon il affiche juste le job qui est en cours d'execution et le laisse travailler en arriére plan, s'il y a une redirection dans la commande alors on execute la fonction run_redirection qui se trouve dans le fichier redirection.c qui fonctionne de la même maniére que la fonction run_extern_command sauf qu'elle a une étape en plus c'est de gérer toutes les redirections qu'elle croise dans la ligne de commande et s'il y a une erreure la commande ne réussit pas et on renvoie 1 à l'utilisateur avec un message qui décrit pourquoi la redirection n'a pas fonctionné en remettant les descripteurs 0, 1 et 2 à leurs valeurs par défaut, sinon si la commande est un pipeline alors on appelle la fonction run_pipe qui compte le nombre de '|' dans la commande qui ne sont pas à l'intérieur des substitutions des processus (entre "<(" et ")") pour avoir le nombre de tubes à créer et le nombre de commande dans le pipeline, puis parse la commande avec la fonction parse_pipe qui renvoie un tableau de tokens pour chaque sous commande dans le pipeline, et pour chaque sous commande on fait un fork et le fils ferme les discripteurs inutiles pour l'execution de sa commande fait les bonnes redirections de son entré et sa sortie standard et traite aussi les redirections qui peuvent être dans sa commande puis teste le type de la commande par exemple si c'est une substitution de processus alors on appelle la fonction run_process_substitution_aux qui va éxécuter la commande et elle prend en paramétre le job que le pipeline a créer pour ajouter chacun des processus de la substitution dans le job et c'est uniquement le shell qui surveille les jobs et pas ces fonctions et si c'est une commande externe on utilise juste un simple exec et dans le père on initialise le leader du job au premier processus fils et si le job est en avant plan on lui donne accés au terminal et pour tout les autres processus fils on les ajoute au groupe du leader et on les ajoute aussi au processus du job, si le job est en avant plan on fait une attente bloquante sinon of affiche juste le job et on redonne la main au shell, sur le même principe fonctionne aussi la fonction run_process_substitution; aprés chaque tour de boucle dans la boucle principale du shell on fait une attente non_bloquante sur tous les processus de chaque job avec la fonction check_jobs qui se trouve dans le fichier jobs_supervisor.h et met à jour l'état de chaque job et affiche les informations nécessaires à l'utilisateur.

# Structure de données

## JobList : 
Structure principale qui maintient la liste des jobs.

typedef struct {
    job_node *head;  // Pointeur vers le premier travail de la liste
    size_t nb_jobs;   // Nombre total de travaux dans la liste
} JobList;

## job_node : 
Représente un travail individuel.

typedef struct job_node {
    struct job_node *next;         // Pointeur vers le prochain job dans la liste
    pid_t pgid;                    // ID du groupe de processus associé au job
    int num_processes;             // Nombre de processus dans le job
    process_node *processes;       // Liste des processus dans le job
    char command[COMMAND_MAXSIZE]; // Commande associée au job
    job_status job_status;          // État du job (RUNNING, DONE, KILLED, etc.)
    lunch_mode mode;               // Mode du job (FOREGROUND ou BACKGROUND)
    unsigned int job_id;           // Identifiant unique du job
} job_node;

## process_node : 
Représente un processus individuel dans un job.

typedef struct process_node {
    struct process_node *next;     // Pointeur vers le prochain processus dans le job
    pid_t pid;                     // ID du processus
    process_status process_status;  // État du processus (RUNNING, DONE, KILLED, etc.)
} process_node;

## Fonctions :

    init_jobs_supervisor() : Initialise le gestionnaire de jobs.

    *add_job(char args) : Ajoute un nouveau job à la liste en fonction de la commande fournis et initialise le mode du job en background si args se termine par & sinon c'est en foreground.

    *add_process(job_node job, pid_t pid) : Ajoute un nouveau processus à un job existant.

    get_job(unsigned int id) : Récupère un job spécifique par son identifiant (%1 %2 etc).

    get_job_of_process(pid_t pid) : Récupère le job auquel appartient un processus spécifique.

    remove_job(unsigned int id) : Supprime un job de la liste en fonction de son identifiant.

    **free_processes_list(process_node processes, job_node job) : Libère la mémoire allouée pour la liste des processus d'un job.

    free_jobs_supervisor() : Libère la mémoire allouée pour le gestionnaire de jobs et ses jobs.

    *display_job(job_node job, int fd) : Affiche les informations sur un job dans un descripteur de fichier donné.

    *update_job(job_node job, pid_t pid, int wstatus, int out_fd) : Met à jour l'état d'un job en fonction du statut d'un de ses processus.

    check_jobs() : Vérifie l'état des jobs en arriére plan et met à jour leurs statuts en conséquence.

# Présentation globale des fichiers

## `extern_commands.c`

Le fichier `extern_commands.c` implémente des fonctions liées à l'exécution de commandes externes dans un shell.

### Fonction `run_extern_command`

La fonction `run_extern_command` est responsable de l'exécution des commandes externes. Voici une description détaillée :

1. **Ajout d'un job** :
   - La fonction commence par appeler la fonction `add_job(args)` pour créer une nouvelle structure de travail (`job_node`) et l'ajouter à la liste des jobs à superviser.

2. **Création d'un processus fils (fork)** :
   - Ensuite, un nouveau processus fils est créé à l'aide de la fonction `fork()`. Ce processus fils sera chargé d'exécuter la commande externe.

3. **Traitement du processus fils** :
   - Dans le processus fils :
     - Les signaux sont réinitialisés en appelant `reset_signals()` pour s'assurer que le fils ne hérite pas des gestionnaires de signaux du shell parent.
     - La commande externe est exécutée en appelant `execvp(args[0], args)`.
     - Si `execvp` échoue (retourne), un message d'erreur est affiché sur la sortie d'erreur standard, et le fils se termine avec le code de sortie `EXIT_FAILURE`.

4. **Traitement du processus parent** :
   - Dans le processus parent (shell) :
     - Le PID du fils est utilisé pour configurer certaines propriétés du job (`job_node`).
     - Si le PID du leader du job (`job->pgid`) n'est pas encore initialisé, il est configuré avec le PID du fils, car le premier processus fils devient le leader du groupe.
     - Si le job est en avant-plan (`job->mode == FOREGROUND`), le terminal est configuré pour utiliser le groupe du job comme groupe de processus courant. Ensuite, le processus parent attend que le processus fils termine ou change de statut (WUNTRACED | WCONTINUED).
     - Si le job est en arrière-plan (`job->mode == BACKGROUND`), le job est affiché et le shell reprend immédiatement sans attendre que le job se termine.

5. **Retour de la fonction** :
   - La fonction retourne `EXIT_SUCCESS` si l'exécution de la commande externe s'est déroulée avec succès, et `EXIT_FAILURE` en cas d'échec.

## `intern_commands.c`

Le fichier `intern_commands.c` implémente des fonctions pour traiter les commandes internes dans un shell.

### Fonction `run_intern_command`

Cette fonction analyse et exécute les commandes internes du shell. Voici un aperçu de chaque commande interne prise en charge :

- **`pwd`** : Affiche le répertoire de travail actuel.

- **`cd`** : Change le répertoire de travail. Si aucun argument n'est fourni, il retourne au répertoire personnel de l'utilisateur. Si l'argument est `-`, il retourne au répertoire précédent.

- **`?`** : Affiche le code de retour de la dernière commande.

- **`exit`** : Termine le shell. Si des jobs sont en cours, un avertissement est affiché. Accepte un argument facultatif pour définir le code de retour du shell.

- **`jobs`** : Affiche la liste des jobs en cours. Accepte des options telles que `-t` pour afficher les détails de chaque job.

- **`bg`** : Relance un job en arrière-plan. Accepte un numéro de job en option.

- **`fg`** : Met un job en avant-plan. Accepte un numéro de job en option.

- **`kill`** : Envoie un signal à un job ou à un processus. Accepte des options et des arguments pour spécifier le signal et la cible.

### Fonctions auxiliaires

Le fichier comprend également plusieurs fonctions auxiliaires :
- `jsh_exit`: Fonction de sortie du shell.
- `pwd`: Fonction pour afficher le répertoire de travail actuel.
- `cd`: Fonction pour changer de répertoire.
- `jobs`: Fonction pour afficher la liste des jobs.
- `bg`: Fonction pour relancer un job en arrière-plan.
- `fg`: Fonction pour mettre un job en avant-plan.
- `kill`: Fonction pour envoyer un signal à un job ou à un processus.
- `job_t`: Fonction pour afficher les détails de tous les jobs.
- `job_t_num`: Fonction pour afficher les détails d'un job spécifique.

## pipeline.c

Le fichier `pipeline.h` contient des fonctions permettant la gestion des pipelines dans un shell. Les pipelines permettent de connecter la sortie d'une commande à l'entrée d'une autre, créant ainsi une séquence de commandes liées.

## Fonction `parse_pipe`

La fonction `parse_pipe` prend en charge l'analyse des tokens d'une commande pour extraire les différentes commandes d'un pipeline. Elle retourne un tableau de tableaux de chaînes de caractères (`char ***`) représentant les commandes individuelles du pipeline.

## Fonction `free_commands`

La fonction `free_commands` libère la mémoire allouée pour les tableaux de commandes après leur utilisation.

## Fonction `run_process_substitution_aux`

Cette fonction prend en charge l'exécution de substitutions de processus. Elle recherche les expressions `<(` dans les tokens et exécute les commandes qu'elles contiennent, en redirigeant leur sortie standard vers des tubes.

## Fonction `run_pipe`

La fonction `run_pipe` gère l'exécution d'un pipeline complet. Elle crée des tubes pour la communication entre les différentes commandes, crée des processus enfants pour chaque commande, et gère les redirections. Cette fonction prend également en charge l'exécution des substitutions de processus lorsqu'elles sont détectées.

# `process_substitution.c`

Le fichier `process_substitution.c` gère l'exécution des substitutions de processus dans le shell. Les substitutions de processus permettent d'intégrer la sortie d'une commande dans un flux de données, rendant ainsi possible leur utilisation comme arguments pour d'autres commandes.

## Fonction `run_process_substitution`

La fonction `run_process_substitution` prend en charge l'exécution des substitutions de processus détectées dans les tokens. Elle crée un travail (`job_node`), analyse les tokens pour déterminer le nombre de pipes nécessaires, et exécute chaque commande en utilisant des processus enfants.

1. **Analyse des tokens** :
   - La fonction identifie les expressions `<(` dans les tokens, qui indiquent le début d'une substitution de processus.
   - Elle compte le nombre de pipes nécessaires et agit en conséquence.

2. **Exécution des commandes** :
   - Si des substitutions de processus sont présentes, la fonction procède à la création de pipes, à la création des processus fils pour exécuter les commandes entre `<(` et `)`, et à la redirection de la sortie standard des commandes vers les pipes.

3. **Attente des processus** :
   - La fonction attend la fin de l'exécution des processus fils, récupérant ainsi les résultats des commandes.
   - En mode avant-plan (`FOREGROUND`), elle attend chaque processus individuellement et met à jour l'état du travail (`job_node`) en conséquence.

4. **Construction des arguments** :
   - La fonction construit le tableau des arguments (`args`) pour la commande qui traitera les résultats des substitutions de processus.

5. **Exécution de la commande résultante** :
   - La fonction crée un dernier processus fils pour exécuter la commande résultante, utilisant le tableau d'arguments construit précédemment.
   - Elle attend la fin de l'exécution de cette commande.

6. **Libération de la mémoire** :
   - La fonction libère la mémoire allouée pour les arguments après leur utilisation.

La fonction retourne `EXIT_SUCCESS` en cas de succès et `EXIT_FAILURE` en cas d'échec.


