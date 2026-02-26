// ============================================================================
// FICHIER: database.c (VERSION FIABILISÉE SANS STRDUP)
// ============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

// --- Fonctions Privées ---

// Fonction pour dupliquer une chaîne en utilisant malloc + strcpy (standard C)
static char* my_strdup(const char* s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char* new_str = malloc(len);
    if (new_str == NULL) return NULL;
    memcpy(new_str, s, len);
    return new_str;
}

static void free_question_content(Question* q) {
    free(q->matiere);
    free(q->chapitre);
    free(q->type);
    free(q->enonce);
    for (int i = 0; i < q->nbChoix; i++) {
        free(q->choix[i]);
    }
    free(q->choix);
}


// --- Fonctions Publiques ---

int load_database(Database* db, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("AVERTISSEMENT: Fichier '%s' non trouve. Une base de donnees vide sera utilisee.\n", filename);
        db->questions = NULL;
        db->count = 0;
        db->capacity = 0;
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0; // Gère \n et \r\n (Windows)

        // On utilise une copie de la ligne pour strtok, car strtok modifie la chaîne
        char line_copy[1024];
        strcpy(line_copy, line);

        char* tokens[7];
        int token_count = 0;
        char* token = strtok(line_copy, ";");
        while (token != NULL && token_count < 7) {
            tokens[token_count++] = token;
            token = strtok(NULL, ";");
        }

        if (token_count >= 6) {
            Question q = {0};
            q.matiere = my_strdup(tokens[0]);
            q.chapitre = my_strdup(tokens[1]);
            q.type = my_strdup(tokens[2]);
            q.enonce = my_strdup(tokens[3]);
            q.bonneReponse = atoi(tokens[4]);
            
            q.points = (token_count == 7) ? atoi(tokens[6]) : 1;

            if (strcmp(tokens[5], "-") != 0) {
                char* choix_str = my_strdup(tokens[5]);
                char* p_choix = strtok(choix_str, "|");
                q.nbChoix = 0;
                q.choix = NULL;
                while (p_choix) {
                    q.choix = realloc(q.choix, sizeof(char*) * (q.nbChoix + 1));
                    q.choix[q.nbChoix++] = my_strdup(p_choix);
                    p_choix = strtok(NULL, "|");
                }
                free(choix_str);
            } else {
                q.choix = NULL;
                q.nbChoix = 0;
            }
            add_question_to_db(db, q);
        }
    }
    fclose(f);
    return 1;
}

void save_database(const Database* db, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Erreur lors de la sauvegarde du fichier");
        return;
    }
    for (int i = 0; i < db->count; i++) {
        Question q = db->questions[i];
        fprintf(f, "%s;%s;%s;%s;%d;", q.matiere, q.chapitre, q.type, q.enonce, q.bonneReponse);
        if (q.nbChoix > 0) {
            for (int j = 0; j < q.nbChoix; j++) {
                fprintf(f, "%s%s", q.choix[j], (j == q.nbChoix - 1) ? "" : "|");
            }
        } else {
            fprintf(f, "-");
        }
        fprintf(f, ";%d\n", q.points);
    }
    fclose(f);
}

void add_question_to_db(Database* db, Question q) {
    if (db->count >= db->capacity) {
        db->capacity = (db->capacity == 0) ? 10 : db->capacity * 2;
        db->questions = realloc(db->questions, db->capacity * sizeof(Question));
        if (!db->questions) {
            perror("Erreur critique de re-allocation memoire");
            exit(EXIT_FAILURE);
        }
    }
    db->questions[db->count++] = q;
}

void free_database(Database* db) {
    for (int i = 0; i < db->count; i++) {
        free_question_content(&db->questions[i]);
    }
    free(db->questions);
    db->questions = NULL;
    db->count = 0;
    db->capacity = 0;
}

void print_database(const Database* db) {
    printf("\n--- Contenu de la base de donnees (%d questions) ---\n", db->count);
    if(db->count == 0) {
        printf("La base de donnees est vide.\n");
    } else {
        for (int i = 0; i < db->count; i++) {
            printf("%d. [%s - %s] (%s): %s\n", i + 1, db->questions[i].matiere, db->questions[i].chapitre, db->questions[i].type, db->questions[i].enonce);
        }
    }
    printf("--------------------------------------------------\n");
}

void delete_question_from_db(Database* db, int index) {
    if (index < 0 || index >= db->count) {
        printf("Erreur : Index de suppression invalide.\n");
        return;
    }

    // 1. Libérer la mémoire de la question que l'on va supprimer
    free_question_content(&db->questions[index]);

    // 2. Décaler tous les éléments suivants vers la gauche pour combler le trou
    // memmove est plus sûr que memcpy pour les zones qui se chevauchent.
    int num_to_move = db->count - index - 1;
    if (num_to_move > 0) {
        memmove(&db->questions[index], &db->questions[index + 1], num_to_move * sizeof(Question));
    }

    // 3. Mettre à jour le nombre total de questions
    db->count--;

    printf("Question supprimee avec succes.\n");
}

// Fonction utilitaire pour vérifier si une chaîne est déjà dans un tableau de chaînes
static int is_in_array(char** array, int count, const char* value) {
    for (int i = 0; i < count; i++) {
        if (strcmp(array[i], value) == 0) {
            return 1; // Trouvé
        }
    }
    return 0; // Non trouvé
}

char** get_unique_subjects(const Database* db, int* count) {
    char** subjects = NULL;
    *count = 0;
    for (int i = 0; i < db->count; i++) {
        if (!is_in_array(subjects, *count, db->questions[i].matiere)) {
            subjects = realloc(subjects, sizeof(char*) * (*count + 1));
            subjects[*count] = db->questions[i].matiere; // On pointe directement, pas de copie
            (*count)++;
        }
    }
    return subjects;
}

char** get_unique_chapters(const Database* db, const char* subject, int* count) {
    char** chapters = NULL;
    *count = 0;
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->questions[i].matiere, subject) == 0) {
            if (!is_in_array(chapters, *count, db->questions[i].chapitre)) {
                chapters = realloc(chapters, sizeof(char*) * (*count + 1));
                chapters[*count] = db->questions[i].chapitre;
                (*count)++;
            }
        }
    }
    return chapters;
}

// NOTE: Cette fonction libère uniquement le tableau de pointeurs, pas les chaînes elles-mêmes,
// car elles pointent vers les données de la base de données principale.
void free_string_array(char** array, int count) {
    free(array);
}

void update_question_in_db(Database* db, int index, Question new_question) {
    if (index < 0 || index >= db->count) {
        printf("Erreur : Index de mise a jour invalide.\n");
        return;
    }
    
    // Free old question content
    free_question_content(&db->questions[index]);
    
    // Replace with new question
    db->questions[index] = new_question;
    
    printf("Question mise a jour avec succes.\n");
}
