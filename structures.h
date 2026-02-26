// structures.h
#ifndef STRUCTURES_H
#define STRUCTURES_H

typedef enum {
    EXAM_TYPE_QCM_ONLY,    // 20 QCM questions
    EXAM_TYPE_MIXED        // 10 QCM + 1 exercise
} ExamType;

// Structure pour une seule question (plus flexible)
typedef struct {
    char* matiere;
    char* chapitre;
    char* type;        // "QCM" ou "Exercice"
    char* enonce;
    char** choix;      // Tableau de chaînes pour les choix
    int nbChoix;
    int bonneReponse;  // Index de la bonne réponse (à partir de 0), -1 si exercice
    int points;        // Added points field for scoring
} Question;

// Structure pour gérer la collection de questions en mémoire
typedef struct {
    Question* questions; // Tableau dynamique de questions
    int count;           // Nombre de questions actuellement dans le tableau
    int capacity;        // Capacité actuelle du tableau
} Database;

#endif
