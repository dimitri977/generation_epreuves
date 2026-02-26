// generator.h
#ifndef GENERATOR_H
#define GENERATOR_H

#include "structures.h"

void generate_exam(const Database* db, const char* matiere, const char* chapitre, 
                   ExamType exam_type, const char* output_filename, const char* format);

#endif
