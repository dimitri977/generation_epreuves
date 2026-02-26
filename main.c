// ============================================================================
// FICHIER : main.c (VERSION FINALE AVEC CRUD COMPLET)
// ============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "structures.h"
#include "database.h"
#include "generator.h"

#define DB_FILE "questions.txt"

// --- Fonctions Utilitaires (Helpers) ---

void clean_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

char* my_strdup(const char* s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char* new_str = malloc(len);
    if (new_str == NULL) return NULL;
    memcpy(new_str, s, len);
    return new_str;
}

char* select_or_create_string(char** existing_items, int count, const char* prompt) {
    printf("--- Choix %s ---\n", prompt);
    for (int i = 0; i < count; i++) {
        printf("%d. %s\n", i + 1, existing_items[i]);
    }
    printf("-------------------\n");
    printf("%d. Creer un(e) nouveau(elle) %s\n", count + 1, prompt);
    printf("Votre choix : ");

    int choice;
    if (scanf("%d", &choice) != 1) { choice = -1; }
    clean_stdin();

    if (choice > 0 && choice <= count) {
        return my_strdup(existing_items[choice - 1]);
    } else if (choice == count + 1) {
        char buffer[100];
        printf("Entrez le nom du/de la nouveau(elle) %s : ", prompt);
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        return my_strdup(buffer);
    } else {
        printf("Choix invalide.\n");
        return NULL;
    }
}

// --- Fonction Principale ---

int main() {
    srand(time(NULL));
    Database db = {0};
    load_database(&db, DB_FILE);
    printf("Base de donnees '%s' chargee. %d questions trouvees.\n", DB_FILE, db.count);

    int choix;
    do {
        printf("\n GENERATEUR D'EPREUVES - MENU PRINCIPAL \n");
        printf("1. Ajouter une question\n");
        printf("2. Lister toutes les questions\n");
        printf("3. Supprimer une question\n");
        printf("4. Modifier une question\n");
        printf("5. Generer une epreuve\n");
        printf("\n");
        printf("9. Sauvegarder et Quitter\n");
        printf("0. Quitter sans sauvegarder\n");
        printf("Votre choix : ");

        if (scanf("%d", &choix) != 1) { choix = -1; }
        clean_stdin();

        switch(choix) {
            case 1: { // AJOUTER
                // ... (Ce code est d�j� bon et ne change pas) ...
                Question q = {0}; char buffer[1024];
                int subject_count = 0; char** subjects = get_unique_subjects(&db, &subject_count);
                q.matiere = select_or_create_string(subjects, subject_count, "Matiere");
                free_string_array(subjects, subject_count);
                if (!q.matiere) break;
                int chapter_count = 0; char** chapters = get_unique_chapters(&db, q.matiere, &chapter_count);
                q.chapitre = select_or_create_string(chapters, chapter_count, "Chapitre");
                free_string_array(chapters, chapter_count);
                if (!q.chapitre) { free(q.matiere); break; }
                printf("Type (QCM/Ouverte) : "); fgets(buffer, sizeof(buffer), stdin); buffer[strcspn(buffer, "\n")] = 0; q.type = my_strdup(buffer);
                printf("Enonce : "); fgets(buffer, sizeof(buffer), stdin); buffer[strcspn(buffer, "\n")] = 0; q.enonce = my_strdup(buffer);
                if (strcmp(q.type, "QCM") == 0) {
                    printf("Choix (separes par |) : "); fgets(buffer, sizeof(buffer), stdin); buffer[strcspn(buffer, "\n")] = 0;
                    char* p_choix = strtok(buffer, "|");
                    while(p_choix) {
                        q.choix = realloc(q.choix, sizeof(char*) * (q.nbChoix + 1)); q.choix[q.nbChoix++] = my_strdup(p_choix); p_choix = strtok(NULL, "|");
                    }
                    printf("Numero de la bonne reponse (commence a 1) : "); scanf("%d", &q.bonneReponse); q.bonneReponse--; clean_stdin();
                } else { q.bonneReponse = -1; }
                add_question_to_db(&db, q); printf("Question ajoutee avec succes !\n");
                break;
            }
            case 2: { // LISTER
                print_database(&db);
                break;
            }
            case 3: { // SUPPRIMER
                 // ... (Ce code est d�j� bon et ne change pas) ...
                if (db.count == 0) { printf("La base de donnees est vide.\n"); break; }
                print_database(&db); printf("Entrez le numero de la question a supprimer (1 a %d) : ", db.count);
                int num_to_delete; scanf("%d", &num_to_delete); clean_stdin();
                if (num_to_delete > 0 && num_to_delete <= db.count) { delete_question_from_db(&db, num_to_delete - 1); }
                else { printf("Numero invalide.\n"); }
                break;
            }
            case 4: { // MODIFIER (VERSION COMPL�TE)
                if (db.count == 0) { printf("La base de donnees est vide.\n"); break; }
                print_database(&db);
                printf("Entrez le numero de la question a modifier (1 a %d) : ", db.count);
                int num_to_edit;
                scanf("%d", &num_to_edit); clean_stdin();

                if (num_to_edit < 1 || num_to_edit > db.count) {
                    printf("Numero invalide.\n");
                    break;
                }
                Question* q_ptr = &db.questions[num_to_edit - 1];
                Question old_q = *q_ptr; // On fait une copie de l'ancienne question

                printf("\n Modification de la question %d   \n", num_to_edit);
                printf("Laissez une reponse vide pour conserver la valeur actuelle.\n\n");

                char buffer[1024];

                // --- Modification de l'�nonc� ---
                printf("Nouvel enonce (actuel: %s) : ", old_q.enonce);
                fgets(buffer, sizeof(buffer), stdin); buffer[strcspn(buffer, "\n")] = 0;
                if (strlen(buffer) == 0) { // L'utilisateur n'a rien tap�
                    strcpy(buffer, old_q.enonce); // On garde l'ancien
                }
                q_ptr->enonce = my_strdup(buffer);

                // --- Modification du type ---
                printf("Nouveau type (actuel: %s) [QCM/Ouverte] : ", old_q.type);
                fgets(buffer, sizeof(buffer), stdin); buffer[strcspn(buffer, "\n")] = 0;
                if (strlen(buffer) == 0) {
                    strcpy(buffer, old_q.type);
                }
                q_ptr->type = my_strdup(buffer);

                // --- Modification des choix et de la r�ponse ---
                if (strcmp(q_ptr->type, "QCM") == 0) {
                    printf("Nouveaux choix separes par | (laissez vide pour garder les anciens) : ");
                    fgets(buffer, sizeof(buffer), stdin); buffer[strcspn(buffer, "\n")] = 0;

                    // Lib�re les anciens choix s'il y en avait
                    for(int i = 0; i < old_q.nbChoix; i++) free(old_q.choix[i]);
                    free(old_q.choix);
                    q_ptr->choix = NULL;
                    q_ptr->nbChoix = 0;

                    if (strlen(buffer) > 0) { // L'utilisateur a entr� de nouveaux choix
                        char* p_choix = strtok(buffer, "|");
                        while(p_choix) {
                           q_ptr->choix = realloc(q_ptr->choix, sizeof(char*) * (q_ptr->nbChoix + 1));
                           q_ptr->choix[q_ptr->nbChoix++] = my_strdup(p_choix);
                           p_choix = strtok(NULL, "|");
                        }
                    } else { // Garder les anciens choix
                        q_ptr->choix = old_q.choix;
                        q_ptr->nbChoix = old_q.nbChoix;
                        old_q.choix = NULL; // �vite la double lib�ration
                    }

                    printf("Nouvelle bonne reponse (actuel: %d) : ", old_q.bonneReponse + 1);
                    fgets(buffer, sizeof(buffer), stdin); buffer[strcspn(buffer, "\n")] = 0;
                    if (strlen(buffer) > 0) {
                        q_ptr->bonneReponse = atoi(buffer) - 1;
                    } else {
                        q_ptr->bonneReponse = old_q.bonneReponse;
                    }

                } else { // C'est une question ouverte
                    // Si c'�tait un QCM avant, on nettoie
                    for(int i = 0; i < old_q.nbChoix; i++) free(old_q.choix[i]);
                    free(old_q.choix);
                    q_ptr->choix = NULL;
                    q_ptr->nbChoix = 0;
                    q_ptr->bonneReponse = -1;
                }

                // --- Lib�rer le reste de l'ancienne question ---
                free(old_q.matiere);
                free(old_q.chapitre);
                free(old_q.type);
                free(old_q.enonce);
                // Si on a gard� les choix, old_q.choix a �t� mis � NULL
                // sinon, on les a d�j� lib�r�s plus haut.
                free(old_q.choix);

                // Pour la mati�re et le chapitre, on garde simplement les anciens pour l'instant
                // L'ajout d'un menu de s�lection ici est possible mais complexifierait davantage.
                q_ptr->matiere = my_strdup(db.questions[num_to_edit - 1].matiere);
                q_ptr->chapitre = my_strdup(db.questions[num_to_edit - 1].chapitre);

                printf("Question modifiee avec succes.\n");
                break;
            }
            case 5: { // GENERER EPREUVE
                // ... (Pas de changement) ...
                char matiere[100], chapitre[100], filename[100]; int nbQCM, nbOuvertes;
                printf("Matiere de l'epreuve : "); fgets(matiere, sizeof(matiere), stdin); matiere[strcspn(matiere, "\n")] = 0;
                printf("Chapitre (laisser vide pour tous les chapitres) : "); fgets(chapitre, sizeof(chapitre), stdin); chapitre[strcspn(chapitre, "\n")] = 0;
                printf("Nombre de QCM : "); scanf("%d", &nbQCM); clean_stdin();
                printf("Nombre de questions ouvertes : "); scanf("%d", &nbOuvertes); clean_stdin();
                printf("Nom du fichier de sortie (ex: epreuve_maths.txt) : "); fgets(filename, sizeof(filename), stdin); filename[strcspn(filename, "\n")] = 0;
                generate_exam(&db, matiere, chapitre, nbQCM, nbOuvertes, filename);
                break;
            }
            case 9: { // SAUVEGARDER ET QUITTER
                save_database(&db, DB_FILE);
                printf("Base de donnees sauvegardee dans '%s'.\n", DB_FILE);
                choix = 0;
                break;
            }
            case 0:
                printf("Au revoir ! (Les modifications non sauvegardees seront perdues)\n");
                break;
            default:
                printf("Choix invalide. Veuillez reessayer.\n");
                break;
        }

    } while (choix != 0);

    free_database(&db);
    return 0;
}
