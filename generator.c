// generator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cairo.h>
#include <cairo-pdf.h>
#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif
#include "generator.h"

// Fonction pour mélanger un tableau d'indices (Fisher-Yates shuffle)
void shuffle(int *array, size_t n) {
    if (n > 1) {
        for (size_t i = 0; i < n - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

static double draw_wrapped_text(cairo_t *cr, const char *text, double x, double y, double max_width, double line_height) {
    if (!text || strlen(text) == 0) return y;
    
    char *text_copy = strdup(text);
    char *line_start = text_copy;
    char *current = text_copy;
    double current_y = y;
    int text_rendered = 0;
    
    while (*current) {
        char *word_end = current;
        while (*word_end && *word_end != ' ' && *word_end != '\n') word_end++;
        
        char saved = *word_end;
        *word_end = '\0';
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, line_start, &extents);
        
        if (extents.width > max_width && current != line_start) {
            *(current - 1) = '\0';
            cairo_move_to(cr, x, current_y);
            cairo_show_text(cr, line_start);
            current_y += line_height;
            line_start = current;
            *word_end = saved;
            text_rendered = 0;
        } else {
            *word_end = saved;
            if (*word_end == '\n' || *word_end == '\0') {
                cairo_move_to(cr, x, current_y);
                cairo_show_text(cr, line_start);
                current_y += line_height;
                text_rendered = 1;
                if (*word_end == '\n') {
                    current = word_end + 1;
                    line_start = current;
                    text_rendered = 0;
                } else {
                    break;
                }
            } else {
                current = word_end + 1;
                text_rendered = 0;
            }
        }
    }
    
    if (!text_rendered && line_start < current && *line_start != '\0') {
        cairo_move_to(cr, x, current_y);
        cairo_show_text(cr, line_start);
        current_y += line_height;
    }
    
    free(text_copy);
    return current_y;
}

// PDF generation function
static void generate_exam_pdf(const Database* db, const char* matiere, const char* chapitre,
                              ExamType exam_type, const char* full_path,
                              int* qcm_indices, int* exercice_indices,
                              int nbQCM, int nbExercice, double points_per_qcm, int points_per_exercice) {
    cairo_surface_t *surface = cairo_pdf_surface_create(full_path, 595, 842); // A4 size
    cairo_t *cr = cairo_create(surface);
    
    double margin = 50;
    double page_width = 595 - 2 * margin;
    double y = margin;
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    // Header with gradient effect
    cairo_set_source_rgb(cr, 0.12, 0.25, 0.69); // Blue color
    cairo_rectangle(cr, 0, 0, 595, 80);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20);
    cairo_move_to(cr, margin, 35);
    cairo_show_text(cr, "EPREUVE D'INGENIERIE INFORMATIQUE");
    
    y = 100;
    
    // Exam details
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, 12);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Matiere : %s", matiere);
    cairo_move_to(cr, margin, y);
    cairo_show_text(cr, buffer);
    y += 20;
    
    snprintf(buffer, sizeof(buffer), "Chapitre : %s", (chapitre && strlen(chapitre) > 0) ? chapitre : "Tous");
    cairo_move_to(cr, margin, y);
    cairo_show_text(cr, buffer);
    y += 20;
    
    snprintf(buffer, sizeof(buffer), "Type d'epreuve : %s", 
             exam_type == EXAM_TYPE_QCM_ONLY ? "QCM Uniquement" : "Mixte (QCM + Exercice)");
    cairo_move_to(cr, margin, y);
    cairo_show_text(cr, buffer);
    y += 20;
    
    cairo_move_to(cr, margin, y);
    cairo_show_text(cr, "Note totale : 20 points");
    y += 20;
    
    snprintf(buffer, sizeof(buffer), "Date : %02d/%02d/%04d", 
             t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
    cairo_move_to(cr, margin, y);
    cairo_show_text(cr, buffer);
    y += 30;
    
    // Separator line
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, margin, y);
    cairo_line_to(cr, 595 - margin, y);
    cairo_stroke(cr);
    y += 25;
    
    // QCM Section
    cairo_set_source_rgb(cr, 0.12, 0.25, 0.69);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, margin, y);
    cairo_show_text(cr, "PARTIE 1 : QUESTIONS A CHOIX MULTIPLES");
    y += 20;
    
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
    cairo_set_font_size(cr, 10);
    snprintf(buffer, sizeof(buffer), "(%d questions - %.1f point%s chacune)", 
             nbQCM, points_per_qcm, points_per_qcm > 1 ? "s" : "");
    cairo_move_to(cr, margin, y);
    cairo_show_text(cr, buffer);
    y += 25;
    
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11);
    
    for (int i = 0; i < nbQCM; i++) {
        if (y > 750) { // New page if needed
            cairo_show_page(cr);
            y = margin;
        }
        
        Question q = db->questions[qcm_indices[i]];
        
        // Question header
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        snprintf(buffer, sizeof(buffer), "Question %d (%.1f point%s) :", 
                 i + 1, points_per_qcm, points_per_qcm > 1 ? "s" : "");
        cairo_move_to(cr, margin, y);
        cairo_show_text(cr, buffer);
        y += 20;
        
        // Question text with proper wrapping
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        y = draw_wrapped_text(cr, q.enonce, margin + 5, y, page_width - 10, 16);
        y += 10; // Space after question text
        
        // Choices
        for (int j = 0; j < q.nbChoix; j++) {
            if (y > 780) { // Check for page break
                cairo_show_page(cr);
                y = margin;
            }
            snprintf(buffer, sizeof(buffer), "%c) %s", 'A' + j, q.choix[j]);
            y = draw_wrapped_text(cr, buffer, margin + 15, y, page_width - 20, 16);
            y += 5; // Small space between choices
        }
        
        // Answer line
        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
        cairo_move_to(cr, margin + 15, y);
        cairo_show_text(cr, "Reponse : _____");
        y += 25;
        
        // Separator
        cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
        cairo_set_line_width(cr, 0.5);
        cairo_move_to(cr, margin, y);
        cairo_line_to(cr, 595 - margin, y);
        cairo_stroke(cr);
        y += 20;
    }
    
    if (nbExercice > 0) {
        if (y > 600) { // Ensure enough space for exercise
            cairo_show_page(cr);
            y = margin;
        }
        
        y += 15;
        cairo_set_source_rgb(cr, 0.12, 0.25, 0.69);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, margin, y);
        cairo_show_text(cr, "PARTIE 2 : EXERCICE");
        y += 20;
        
        cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
        cairo_set_font_size(cr, 10);
        snprintf(buffer, sizeof(buffer), "(%d point%s)", 
                 points_per_exercice, points_per_exercice > 1 ? "s" : "");
        cairo_move_to(cr, margin, y);
        cairo_show_text(cr, buffer);
        y += 25;
        
        Question q = db->questions[exercice_indices[0]];
        
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 11);
        snprintf(buffer, sizeof(buffer), "Exercice (%d points) :", points_per_exercice);
        cairo_move_to(cr, margin, y);
        cairo_show_text(cr, buffer);
        y += 20;
        
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        y = draw_wrapped_text(cr, q.enonce, margin + 5, y, page_width - 10, 16);
        y += 25;
        
        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
        cairo_move_to(cr, margin + 5, y);
        cairo_show_text(cr, "Reponse :");
        y += 20;
        
        // Answer lines
        for (int i = 0; i < 5; i++) {
            if (y > 800) {
                cairo_show_page(cr);
                y = margin;
            }
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
            cairo_set_line_width(cr, 0.5);
            cairo_move_to(cr, margin + 5, y);
            cairo_line_to(cr, 595 - margin - 5, y);
            cairo_stroke(cr);
            y += 25;
        }
    }
    
    // Footer
    if (y > 750) {
        cairo_show_page(cr);
    }
    
    y = 800;
    cairo_set_source_rgb(cr, 0.12, 0.25, 0.69);
    cairo_rectangle(cr, 0, y, 595, 42);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, 200, y + 25);
    cairo_show_text(cr, "FIN DE L'EPREUVE");
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void generate_exam(const Database* db, const char* matiere, const char* chapitre, 
                   ExamType exam_type, const char* output_filename, const char* format) {
    const char* output_dir = "Epreuves_Generees";
    mkdir(output_dir, 0777);
    
    char base_filename[256];
    const char* dot = strrchr(output_filename, '.');
    if (dot) {
        size_t len = dot - output_filename;
        strncpy(base_filename, output_filename, len);
        base_filename[len] = '\0';
    } else {
        strcpy(base_filename, output_filename);
    }
    
    char full_path[512];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    
    const char* extension = (strcmp(format, "PDF") == 0) ? "pdf" : "txt";
    snprintf(full_path, sizeof(full_path), "%s/%s_%s.%s", 
             output_dir, base_filename, timestamp, extension);
    
    int* qcm_indices = malloc(db->count * sizeof(int));
    int* exercice_indices = malloc(db->count * sizeof(int));
    int cQCM = 0, cExercice = 0;

    char** used_qcm_enonces = malloc(db->count * sizeof(char*));
    char** used_exercice_enonces = malloc(db->count * sizeof(char*));
    int used_qcm_count = 0;
    int used_exercice_count = 0;

    int chapitre_is_optional = (chapitre == NULL || strcmp(chapitre, "") == 0);

    for (int i = 0; i < db->count; i++) {
        int matiere_match = (strcmp(db->questions[i].matiere, matiere) == 0);
        int chapitre_match = (chapitre_is_optional || strcmp(db->questions[i].chapitre, chapitre) == 0);

        if (matiere_match && chapitre_match) {
            if (strcmp(db->questions[i].type, "QCM") == 0) {
                int is_duplicate = 0;
                for (int j = 0; j < used_qcm_count; j++) {
                    if (strcmp(db->questions[i].enonce, used_qcm_enonces[j]) == 0) {
                        is_duplicate = 1;
                        break;
                    }
                }
                if (!is_duplicate) {
                    qcm_indices[cQCM] = i;
                    used_qcm_enonces[used_qcm_count] = db->questions[i].enonce;
                    cQCM++;
                    used_qcm_count++;
                }
            } else if (strcmp(db->questions[i].type, "Exercice") == 0) {
                int is_duplicate = 0;
                for (int j = 0; j < used_exercice_count; j++) {
                    if (strcmp(db->questions[i].enonce, used_exercice_enonces[j]) == 0) {
                        is_duplicate = 1;
                        break;
                    }
                }
                if (!is_duplicate) {
                    exercice_indices[cExercice] = i;
                    used_exercice_enonces[used_exercice_count] = db->questions[i].enonce;
                    cExercice++;
                    used_exercice_count++;
                }
            }
        }
    }

    free(used_qcm_enonces);
    free(used_exercice_enonces);

    int nbQCM = 0, nbExercice = 0;
    double points_per_qcm = 0;
    int points_per_exercice = 0;
    
    if (exam_type == EXAM_TYPE_QCM_ONLY) {
        nbQCM = 20;
        nbExercice = 0;
        points_per_qcm = 1.0;
    } else {
        nbQCM = 10;
        nbExercice = 1;
        points_per_qcm = 0.5;
        points_per_exercice = 15;
    }

    if (cQCM < nbQCM || (nbExercice > 0 && cExercice < nbExercice)) {
        printf("\n=== ERREUR DE GENERATION ===\n");
        printf("Pas assez de questions uniques dans la base de donnees pour generer l'epreuve.\n\n");
        printf("Filtre applique :\n");
        printf("  - Matiere: '%s'\n", matiere);
        printf("  - Chapitre: '%s'\n", chapitre_is_optional ? "(tous)" : chapitre);
        printf("\nQuestions uniques disponibles :\n");
        printf("  - QCM disponibles: %d (requis: %d)\n", cQCM, nbQCM);
        printf("  - Exercices disponibles: %d (requis: %d)\n", cExercice, nbExercice);
        printf("\nVeuillez ajouter plus de questions uniques pour cette matiere dans la base de donnees.\n");
        printf("============================\n\n");
        free(qcm_indices);
        free(exercice_indices);
        return;
    }

    shuffle(qcm_indices, cQCM);
    shuffle(exercice_indices, cExercice);

    if (strcmp(format, "PDF") == 0) {
        generate_exam_pdf(db, matiere, chapitre, exam_type, full_path,
                         qcm_indices, exercice_indices, nbQCM, nbExercice,
                         points_per_qcm, points_per_exercice);
    } else {
        // TXT generation code
        FILE *f = fopen(full_path, "w");
        if (!f) {
            perror("Impossible de creer le fichier d'examen");
            free(qcm_indices);
            free(exercice_indices);
            return;
        }

        fprintf(f, "========================================\n");
        fprintf(f, "       EPREUVE D'INGENIERIE INFORMATIQUE\n");
        fprintf(f, "========================================\n\n");
        fprintf(f, "Matiere : %s\n", matiere);
        fprintf(f, "Chapitre : %s\n", chapitre_is_optional ? "Tous" : chapitre);
        fprintf(f, "Type d'epreuve : %s\n", exam_type == EXAM_TYPE_QCM_ONLY ? "QCM Uniquement" : "Mixte (QCM + Exercice)");
        fprintf(f, "Note totale : 20 points\n");
        fprintf(f, "Date de generation : %02d/%02d/%04d %02d:%02d:%02d\n", 
                t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
                t->tm_hour, t->tm_min, t->tm_sec);
        fprintf(f, "========================================\n\n");

        int question_num = 1;
        
        fprintf(f, "PARTIE 1 : QUESTIONS A CHOIX MULTIPLES\n");
        fprintf(f, "---------------------------------------\n");
        fprintf(f, "(%d questions - %.1f point%s chacune)\n\n", nbQCM, points_per_qcm, points_per_qcm > 1 ? "s" : "");
        
        for (int i = 0; i < nbQCM; i++) {
            Question q = db->questions[qcm_indices[i]];
            fprintf(f, "Question %d (%.1f point%s) :\n", question_num++, points_per_qcm, points_per_qcm > 1 ? "s" : "");
            fprintf(f, "%s\n\n", q.enonce);
            for (int j = 0; j < q.nbChoix; j++) {
                fprintf(f, "   %c) %s\n", 'A' + j, q.choix[j]);
            }
            fprintf(f, "\nReponse : _____\n\n");
            fprintf(f, "---------------------------------------\n\n");
        }

        if (nbExercice > 0) {
            fprintf(f, "\nPARTIE 2 : EXERCICE\n");
            fprintf(f, "---------------------------------------\n");
            fprintf(f, "(%d point%s)\n\n", points_per_exercice, points_per_exercice > 1 ? "s" : "");
            
            Question q = db->questions[exercice_indices[0]];
            fprintf(f, "Exercice (%d points) :\n", points_per_exercice);
            fprintf(f, "%s\n\n", q.enonce);
            fprintf(f, "Reponse :\n");
            fprintf(f, "____________________________________________________________\n\n");
            fprintf(f, "____________________________________________________________\n\n");
            fprintf(f, "____________________________________________________________\n\n");
            fprintf(f, "____________________________________________________________\n\n");
            fprintf(f, "____________________________________________________________\n\n");
        }

        fprintf(f, "\n========================================\n");
        fprintf(f, "              FIN DE L'EPREUVE\n");
        fprintf(f, "========================================\n");

        fclose(f);
    }

    free(qcm_indices);
    free(exercice_indices);
    
    printf("\n=== EPREUVE GENEREE AVEC SUCCES ===\n");
    printf("Fichier : %s\n", full_path);
    printf("Format : %s\n", extension);
    printf("Type : %s\n", exam_type == EXAM_TYPE_QCM_ONLY ? "QCM Uniquement (20 questions)" : "Mixte (10 QCM + 1 Exercice)");
    printf("===================================\n\n");
}
