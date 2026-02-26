// database.h
#ifndef DATABASE_H
#define DATABASE_H

#include "structures.h"

// Charge les questions depuis le fichier dans la structure Database
int load_database(Database* db, const char* filename);

// Sauvegarde la base de donnes en mmoire dans le fichier
void save_database(const Database* db, const char* filename);

// Ajoute une question la base de donnes en mmoire
void add_question_to_db(Database* db, Question q);

// Libre toute la mmoire alloue pour la base de donnes
void free_database(Database* db);

// Affiche toutes les questions (pour le dbogage/vrification)
void print_database(const Database* db);

// Supprime une question de la base de donnes un index donn
void delete_question_from_db(Database* db, int index);

// Extrait une liste de matires uniques depuis la base de donnes
char** get_unique_subjects(const Database* db, int* count);

// Extrait une liste de chapitres uniques pour une matire donne
char** get_unique_chapters(const Database* db, const char* subject, int* count);

// Libre la mmoire alloue par les fonctions ci-dessus
void free_string_array(char** array, int count);

// Met jour une question dans la base de donnes en mmoire
void update_question_in_db(Database* db, int index, Question new_question);

#endif
