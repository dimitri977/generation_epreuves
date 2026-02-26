// gui_main.c - Modern GTK4 Interface for Exam Generator
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "structures.h"
#include "database.h"
#include "generator.h"

#define DB_FILE "questions.txt"
#define PASSWORD "12345"

static const char* COMPUTER_ENGINEERING_SUBJECTS[] = {
    "Programmation",
    "Algorithmes et Structures de Données",
    "Bases de Données",
    "Réseaux Informatiques",
    "Systèmes d'Exploitation",
    "Architecture des Ordinateurs",
    "Génie Logiciel",
    "Intelligence Artificielle",
    "Sécurité Informatique",
    "Développement Web",
    "Développement Mobile",
    "Cloud Computing",
    "Big Data et Analytics",
    "Systèmes Embarqués",
    NULL
};

typedef struct {
    Database db;
    GtkWidget *main_window;
    GtkWidget *stack;
    GtkWidget *list_view;
    GtkWidget *status_label;
    GtkWidget *count_label;
    int selected_question_index;
    GtkWidget *login_window;
    GtkApplication *gtk_app;
} AppData;

typedef struct {
    char** chapters;
    int* selected;
    int count;
} ChapterSelection;

static void show_notification(AppData *app, const char *message, const char *type);
static void refresh_question_list(AppData *app);
static void switch_to_view(AppData *app, const char *view_name);
static void on_add_question_clicked(GtkWidget *button, gpointer user_data);
static void on_delete_question_clicked(GtkWidget *button, gpointer user_data);
static void on_edit_question_clicked(GtkWidget *button, gpointer user_data);
static void on_update_question_clicked(GtkWidget *btn, gpointer data);
static void on_generate_exam_clicked(GtkWidget *button, gpointer user_data);
static void on_login_clicked(GtkWidget *button, gpointer user_data);
static void show_login_window(GtkApplication *gtk_app, gpointer user_data);

static char* my_strdup(const char* s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char* new_str = malloc(len);
    if (new_str == NULL) return NULL;
    memcpy(new_str, s, len);
    return new_str;
}

static void show_notification(AppData *app, const char *message, const char *type) {
    if (app->status_label) {
        char markup[512];
        if (strcmp(type, "success") == 0) {
            snprintf(markup, sizeof(markup), "<span foreground='#10b981'>✓ %s</span>", message);
        } else if (strcmp(type, "error") == 0) {
            snprintf(markup, sizeof(markup), "<span foreground='#ef4444'>✗ %s</span>", message);
        } else {
            snprintf(markup, sizeof(markup), "<span foreground='#3b82f6'>ℹ️ %s</span>", message);
        }
        gtk_label_set_markup(GTK_LABEL(app->status_label), markup);
    }
}

static void on_save_question_clicked(GtkWidget *btn, gpointer data) {
    gpointer *params = (gpointer *)data;
    AppData *app = (AppData *)params[0];
    GtkWidget *dialog = (GtkWidget *)params[1];
    GtkWidget *subject_dropdown = (GtkWidget *)params[2];
    GtkWidget *chapter_entry = (GtkWidget *)params[3];
    GtkWidget *type_dropdown = (GtkWidget *)params[4];
    GtkWidget *question_text = (GtkWidget *)params[5];
    GtkWidget *choices_entry = (GtkWidget *)params[6];
    GtkWidget *answer_entry = (GtkWidget *)params[7];

    guint subject_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(subject_dropdown));
    const char *subject = COMPUTER_ENGINEERING_SUBJECTS[subject_idx];
    
    const char *chapter = gtk_editable_get_text(GTK_EDITABLE(chapter_entry));
    guint type_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(type_dropdown));
    const char *type = (type_idx == 0) ? "QCM" : "Exercice";

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(question_text));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *question = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    const char *choices = gtk_editable_get_text(GTK_EDITABLE(choices_entry));
    const char *answer_str = gtk_editable_get_text(GTK_EDITABLE(answer_entry));

    if (strlen(subject) == 0 || strlen(chapter) == 0 || strlen(question) == 0) {
        show_notification(app, "Veuillez remplir tous les champs obligatoires", "error");
        g_free(question);
        return;
    }

    Question q = {0};
    q.matiere = my_strdup(subject);
    q.chapitre = my_strdup(chapter);
    q.type = my_strdup(type);
    q.enonce = my_strdup(question);
    q.points = 1; 

    if (strcmp(type, "QCM") == 0 && strlen(choices) > 0) {
        char *choices_copy = my_strdup(choices);
        char *token = strtok(choices_copy, "|");
        while (token) {
            q.choix = realloc(q.choix, sizeof(char*) * (q.nbChoix + 1));
            q.choix[q.nbChoix++] = my_strdup(token);
            token = strtok(NULL, "|");
        }
        free(choices_copy);
        q.bonneReponse = atoi(answer_str) - 1;
    } else {
        q.bonneReponse = -1;
    }

    add_question_to_db(&app->db, q);
    save_database(&app->db, DB_FILE);

    show_notification(app, "Question ajoutée avec succès", "success");
    refresh_question_list(app);

    if (app->count_label) {
        char count_text[100];
        snprintf(count_text, sizeof(count_text), "%d questions dans la base", app->db.count);
        gtk_label_set_text(GTK_LABEL(app->count_label), count_text);
    }

    g_free(question);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_add_question_clicked(GtkWidget *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Ajouter une Question");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 650, 550);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='x-large' weight='bold'>Nouvelle Question</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *subject_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *subject_label = gtk_label_new("Matière");
    gtk_widget_set_halign(subject_label, GTK_ALIGN_START);
    GtkWidget *subject_dropdown = gtk_drop_down_new_from_strings(COMPUTER_ENGINEERING_SUBJECTS);
    gtk_box_append(GTK_BOX(subject_box), subject_label);
    gtk_box_append(GTK_BOX(subject_box), subject_dropdown);
    gtk_box_append(GTK_BOX(box), subject_box);

    GtkWidget *chapter_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *chapter_label = gtk_label_new("Chapitre");
    gtk_widget_set_halign(chapter_label, GTK_ALIGN_START);
    GtkWidget *chapter_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(chapter_entry), "Ex: Structures de données linéaires");
    gtk_box_append(GTK_BOX(chapter_box), chapter_label);
    gtk_box_append(GTK_BOX(chapter_box), chapter_entry);
    gtk_box_append(GTK_BOX(box), chapter_box);

    GtkWidget *type_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *type_label = gtk_label_new("Type de Question");
    gtk_widget_set_halign(type_label, GTK_ALIGN_START);
    GtkWidget *type_dropdown = gtk_drop_down_new_from_strings((const char *[]){"QCM", "Exercice", NULL});
    gtk_box_append(GTK_BOX(type_box), type_label);
    gtk_box_append(GTK_BOX(type_box), type_dropdown);
    gtk_box_append(GTK_BOX(box), type_box);

    GtkWidget *question_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *question_label = gtk_label_new("Énoncé de la Question");
    gtk_widget_set_halign(question_label, GTK_ALIGN_START);
    GtkWidget *question_text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(question_text), GTK_WRAP_WORD);
    GtkWidget *question_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(question_scroll), 100);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(question_scroll), question_text);
    gtk_box_append(GTK_BOX(question_box), question_label);
    gtk_box_append(GTK_BOX(question_box), question_scroll);
    gtk_box_append(GTK_BOX(box), question_box);

    GtkWidget *choices_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *choices_label = gtk_label_new("Choix (séparés par | - pour QCM uniquement)");
    gtk_widget_set_halign(choices_label, GTK_ALIGN_START);
    GtkWidget *choices_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(choices_entry), "Ex: Réponse A|Réponse B|Réponse C|Réponse D");
    gtk_box_append(GTK_BOX(choices_box), choices_label);
    gtk_box_append(GTK_BOX(choices_box), choices_entry);
    gtk_box_append(GTK_BOX(box), choices_box);

    GtkWidget *answer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *answer_label = gtk_label_new("Numéro de la Bonne Réponse (pour QCM uniquement)");
    gtk_widget_set_halign(answer_label, GTK_ALIGN_START);
    GtkWidget *answer_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(answer_entry), "Ex: 1");
    gtk_box_append(GTK_BOX(answer_box), answer_label);
    gtk_box_append(GTK_BOX(answer_box), answer_entry);
    gtk_box_append(GTK_BOX(box), answer_box);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_widget_set_margin_top(button_box, 10);

    GtkWidget *cancel_btn = gtk_button_new_with_label("Annuler");
    GtkWidget *save_btn = gtk_button_new_with_label("Enregistrer");
    gtk_widget_add_css_class(save_btn, "suggested-action");

    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    gtk_box_append(GTK_BOX(button_box), save_btn);
    gtk_box_append(GTK_BOX(box), button_box);

    gpointer *params = g_new(gpointer, 8);
    params[0] = app;
    params[1] = dialog;
    params[2] = subject_dropdown;
    params[3] = chapter_entry;
    params[4] = type_dropdown;
    params[5] = question_text;
    params[6] = choices_entry;
    params[7] = answer_entry;

    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_question_clicked), params);
    g_signal_connect_swapped(dialog, "destroy", G_CALLBACK(g_free), params);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_delete_response(GtkAlertDialog *dialog, GAsyncResult *result, gpointer data) {
    AppData *app = (AppData *)data;
    int response = gtk_alert_dialog_choose_finish(dialog, result, NULL);

    if (response == 0) {
        delete_question_from_db(&app->db, app->selected_question_index);
        save_database(&app->db, DB_FILE);
        app->selected_question_index = -1;
        show_notification(app, "Question supprimée avec succès", "success");
        refresh_question_list(app);

        if (app->count_label) {
            char count_text[100];
            snprintf(count_text, sizeof(count_text), "%d questions dans la base", app->db.count);
            gtk_label_set_text(GTK_LABEL(app->count_label), count_text);
        }
    }
}

static void on_delete_question_clicked(GtkWidget *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    if (app->selected_question_index < 0 || app->selected_question_index >= app->db.count) {
        show_notification(app, "Veuillez sélectionner une question à supprimer", "error");
        return;
    }

    GtkAlertDialog *dialog = gtk_alert_dialog_new("Êtes-vous sûr de vouloir supprimer cette question ?");
    const char *buttons[] = {"Oui", "Non", NULL};
    gtk_alert_dialog_set_buttons(dialog, buttons);
    gtk_alert_dialog_set_cancel_button(dialog, 1);
    gtk_alert_dialog_set_default_button(dialog, 1);

    gtk_alert_dialog_choose(dialog, GTK_WINDOW(app->main_window), NULL,
                           (GAsyncReadyCallback)on_delete_response, app);
}

static void on_edit_question_clicked(GtkWidget *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    if (app->selected_question_index < 0 || app->selected_question_index >= app->db.count) {
        show_notification(app, "Veuillez sélectionner une question à modifier", "error");
        return;
    }

    Question q = app->db.questions[app->selected_question_index];

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Modifier la Question");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 650, 550);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='x-large' weight='bold'>Modifier la Question</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), title);

    // Subject dropdown
    GtkWidget *subject_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *subject_label = gtk_label_new("Matière");
    gtk_widget_set_halign(subject_label, GTK_ALIGN_START);
    GtkWidget *subject_dropdown = gtk_drop_down_new_from_strings(COMPUTER_ENGINEERING_SUBJECTS);
    
    // Find and set current subject
    for (int i = 0; COMPUTER_ENGINEERING_SUBJECTS[i] != NULL; i++) {
        if (strcmp(COMPUTER_ENGINEERING_SUBJECTS[i], q.matiere) == 0) {
            gtk_drop_down_set_selected(GTK_DROP_DOWN(subject_dropdown), i);
            break;
        }
    }
    
    gtk_box_append(GTK_BOX(subject_box), subject_label);
    gtk_box_append(GTK_BOX(subject_box), subject_dropdown);
    gtk_box_append(GTK_BOX(box), subject_box);

    // Chapter entry
    GtkWidget *chapter_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *chapter_label = gtk_label_new("Chapitre");
    gtk_widget_set_halign(chapter_label, GTK_ALIGN_START);
    GtkWidget *chapter_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(chapter_entry), q.chapitre);
    gtk_box_append(GTK_BOX(chapter_box), chapter_label);
    gtk_box_append(GTK_BOX(chapter_box), chapter_entry);
    gtk_box_append(GTK_BOX(box), chapter_box);

    // Type dropdown
    GtkWidget *type_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *type_label = gtk_label_new("Type de Question");
    gtk_widget_set_halign(type_label, GTK_ALIGN_START);
    GtkWidget *type_dropdown = gtk_drop_down_new_from_strings((const char *[]){"QCM", "Exercice", NULL});
    gtk_drop_down_set_selected(GTK_DROP_DOWN(type_dropdown), strcmp(q.type, "QCM") == 0 ? 0 : 1);
    gtk_box_append(GTK_BOX(type_box), type_label);
    gtk_box_append(GTK_BOX(type_box), type_dropdown);
    gtk_box_append(GTK_BOX(box), type_box);

    // Question text
    GtkWidget *question_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *question_label = gtk_label_new("Énoncé de la Question");
    gtk_widget_set_halign(question_label, GTK_ALIGN_START);
    GtkWidget *question_text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(question_text), GTK_WRAP_WORD);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(question_text));
    gtk_text_buffer_set_text(buffer, q.enonce, -1);
    GtkWidget *question_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(question_scroll), 100);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(question_scroll), question_text);
    gtk_box_append(GTK_BOX(question_box), question_label);
    gtk_box_append(GTK_BOX(question_box), question_scroll);
    gtk_box_append(GTK_BOX(box), question_box);

    // Choices entry
    GtkWidget *choices_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *choices_label = gtk_label_new("Choix (séparés par | - pour QCM uniquement)");
    gtk_widget_set_halign(choices_label, GTK_ALIGN_START);
    GtkWidget *choices_entry = gtk_entry_new();
    
    // Build choices string
    if (q.nbChoix > 0) {
        char choices_str[1024] = "";
        for (int i = 0; i < q.nbChoix; i++) {
            strcat(choices_str, q.choix[i]);
            if (i < q.nbChoix - 1) strcat(choices_str, "|");
        }
        gtk_editable_set_text(GTK_EDITABLE(choices_entry), choices_str);
    }
    
    gtk_box_append(GTK_BOX(choices_box), choices_label);
    gtk_box_append(GTK_BOX(choices_box), choices_entry);
    gtk_box_append(GTK_BOX(box), choices_box);

    // Answer entry
    GtkWidget *answer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *answer_label = gtk_label_new("Numéro de la Bonne Réponse (pour QCM uniquement)");
    gtk_widget_set_halign(answer_label, GTK_ALIGN_START);
    GtkWidget *answer_entry = gtk_entry_new();
    if (q.bonneReponse >= 0) {
        char answer_str[10];
        snprintf(answer_str, sizeof(answer_str), "%d", q.bonneReponse + 1);
        gtk_editable_set_text(GTK_EDITABLE(answer_entry), answer_str);
    }
    gtk_box_append(GTK_BOX(answer_box), answer_label);
    gtk_box_append(GTK_BOX(answer_box), answer_entry);
    gtk_box_append(GTK_BOX(box), answer_box);

    // Buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_widget_set_margin_top(button_box, 10);

    GtkWidget *cancel_btn = gtk_button_new_with_label("Annuler");
    GtkWidget *save_btn = gtk_button_new_with_label("Enregistrer");
    gtk_widget_add_css_class(save_btn, "suggested-action");

    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    gtk_box_append(GTK_BOX(button_box), save_btn);
    gtk_box_append(GTK_BOX(box), button_box);

    gpointer *params = g_new(gpointer, 9);
    params[0] = app;
    params[1] = dialog;
    params[2] = subject_dropdown;
    params[3] = chapter_entry;
    params[4] = type_dropdown;
    params[5] = question_text;
    params[6] = choices_entry;
    params[7] = answer_entry;
    params[8] = GINT_TO_POINTER(app->selected_question_index);

    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_update_question_clicked), params);
    g_signal_connect_swapped(dialog, "destroy", G_CALLBACK(g_free), params);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_update_question_clicked(GtkWidget *btn, gpointer data) {
    gpointer *params = (gpointer *)data;
    AppData *app = (AppData *)params[0];
    GtkWidget *dialog = (GtkWidget *)params[1];
    GtkWidget *subject_dropdown = (GtkWidget *)params[2];
    GtkWidget *chapter_entry = (GtkWidget *)params[3];
    GtkWidget *type_dropdown = (GtkWidget *)params[4];
    GtkWidget *question_text = (GtkWidget *)params[5];
    GtkWidget *choices_entry = (GtkWidget *)params[6];
    GtkWidget *answer_entry = (GtkWidget *)params[7];
    int index = GPOINTER_TO_INT(params[8]);

    guint subject_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(subject_dropdown));
    const char *subject = COMPUTER_ENGINEERING_SUBJECTS[subject_idx];
    
    const char *chapter = gtk_editable_get_text(GTK_EDITABLE(chapter_entry));
    guint type_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(type_dropdown));
    const char *type = (type_idx == 0) ? "QCM" : "Exercice";

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(question_text));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *question = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    const char *choices = gtk_editable_get_text(GTK_EDITABLE(choices_entry));
    const char *answer_str = gtk_editable_get_text(GTK_EDITABLE(answer_entry));

    if (strlen(subject) == 0 || strlen(chapter) == 0 || strlen(question) == 0) {
        show_notification(app, "Veuillez remplir tous les champs obligatoires", "error");
        g_free(question);
        return;
    }

    Question q = {0};
    q.matiere = my_strdup(subject);
    q.chapitre = my_strdup(chapter);
    q.type = my_strdup(type);
    q.enonce = my_strdup(question);
    q.points = 1;

    if (strcmp(type, "QCM") == 0 && strlen(choices) > 0) {
        char *choices_copy = my_strdup(choices);
        char *token = strtok(choices_copy, "|");
        while (token) {
            q.choix = realloc(q.choix, sizeof(char*) * (q.nbChoix + 1));
            q.choix[q.nbChoix++] = my_strdup(token);
            token = strtok(NULL, "|");
        }
        free(choices_copy);
        q.bonneReponse = atoi(answer_str) - 1;
    } else {
        q.bonneReponse = -1;
    }

    update_question_in_db(&app->db, index, q);
    save_database(&app->db, DB_FILE);

    show_notification(app, "Question modifiée avec succès", "success");
    refresh_question_list(app);

    g_free(question);
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_question_item_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
    gpointer *params = (gpointer *)data;
    AppData *app = (AppData *)params[0];
    int index = GPOINTER_TO_INT(params[1]);

    GtkWidget *child = gtk_widget_get_first_child(app->list_view);
    while (child) {
        gtk_widget_remove_css_class(child, "selected");
        child = gtk_widget_get_next_sibling(child);
    }

    GtkWidget *clicked_item = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    gtk_widget_add_css_class(clicked_item, "selected");

    app->selected_question_index = index;
}

static void refresh_question_list(AppData *app) {
    GtkWidget *child = gtk_widget_get_first_child(app->list_view);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(app->list_view), child);
        child = next;
    }

    for (int i = 0; i < app->db.count; i++) {
        Question q = app->db.questions[i];

        GtkWidget *item = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_margin_start(item, 15);
        gtk_widget_set_margin_end(item, 15);
        gtk_widget_set_margin_top(item, 10);
        gtk_widget_set_margin_bottom(item, 10);
        gtk_widget_add_css_class(item, "question-item");

        GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        char markup[512];
        snprintf(markup, sizeof(markup), "<span weight='bold' size='large'>%d.</span> <span weight='bold'>%s - %s</span>",
                 i + 1, q.matiere, q.chapitre);
        GtkWidget *title = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(title), markup);
        gtk_widget_set_halign(title, GTK_ALIGN_START);
        gtk_widget_set_hexpand(title, TRUE);

        GtkWidget *type_badge = gtk_label_new(q.type);
        gtk_widget_add_css_class(type_badge, "badge");
        if (strcmp(q.type, "QCM") == 0) {
            gtk_widget_add_css_class(type_badge, "badge-qcm");
        } else {
            gtk_widget_add_css_class(type_badge, "badge-exercice");
        }

        gtk_box_append(GTK_BOX(header), title);
        gtk_box_append(GTK_BOX(header), type_badge);

        GtkWidget *question_label = gtk_label_new(q.enonce);
        gtk_label_set_wrap(GTK_LABEL(question_label), TRUE);
        gtk_label_set_xalign(GTK_LABEL(question_label), 0);
        gtk_widget_set_margin_top(question_label, 5);

        gtk_box_append(GTK_BOX(item), header);
        gtk_box_append(GTK_BOX(item), question_label);

        GtkGesture *click = gtk_gesture_click_new();
        gpointer *click_params = g_new(gpointer, 2);
        click_params[0] = app;
        click_params[1] = GINT_TO_POINTER(i);
        g_signal_connect(click, "pressed", G_CALLBACK(on_question_item_clicked), click_params);
        g_signal_connect_swapped(item, "destroy", G_CALLBACK(g_free), click_params);
        gtk_widget_add_controller(item, GTK_EVENT_CONTROLLER(click));

        gtk_box_append(GTK_BOX(app->list_view), item);
    }
}

static void on_chapter_checkbox_toggled(GtkCheckButton *checkbox, gpointer data) {
    ChapterSelection *selection = (ChapterSelection *)data;
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(checkbox), "chapter-index"));
    selection->selected[index] = gtk_check_button_get_active(checkbox);
}

static void on_all_chapters_toggled(GtkCheckButton *all_checkbox, gpointer data) {
    GtkWidget *chapter_box = (GtkWidget *)data;
    gboolean all_selected = gtk_check_button_get_active(all_checkbox);
    
    if (all_selected) {
        GtkWidget *child = gtk_widget_get_first_child(chapter_box);
        child = gtk_widget_get_next_sibling(child);
        
        while (child) {
            if (GTK_IS_CHECK_BUTTON(child)) {
                gtk_check_button_set_active(GTK_CHECK_BUTTON(child), FALSE);
            }
            child = gtk_widget_get_next_sibling(child);
        }
    }
}

static void on_individual_chapter_toggled(GtkCheckButton *checkbox, gpointer data) {
    GtkWidget *all_chapters_checkbox = (GtkWidget *)data;
    gboolean is_active = gtk_check_button_get_active(checkbox);
    
    if (is_active) {
        gtk_check_button_set_active(GTK_CHECK_BUTTON(all_chapters_checkbox), FALSE);
    }
}

static void on_subject_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer data) {
    gpointer *params = (gpointer *)data;
    AppData *app = (AppData *)params[0];
    GtkWidget *chapter_box = (GtkWidget *)params[1];
    ChapterSelection *selection = (ChapterSelection *)params[2];
    GtkWidget *all_chapters_checkbox = (GtkWidget *)params[3];
    
    GtkWidget *child = gtk_widget_get_first_child(chapter_box);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(chapter_box), child);
        child = next;
    }
    
    if (selection->chapters) {
        free_string_array(selection->chapters, selection->count);
        free(selection->selected);
    }
    
    guint subject_idx = gtk_drop_down_get_selected(dropdown);
    const char *subject = COMPUTER_ENGINEERING_SUBJECTS[subject_idx];
    
    selection->chapters = get_unique_chapters(&app->db, subject, &selection->count);
    selection->selected = calloc(selection->count, sizeof(int));
    
    GtkWidget *all_checkbox = gtk_check_button_new_with_label("Tous les chapitres");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(all_checkbox), TRUE);
    g_signal_connect(all_checkbox, "toggled", G_CALLBACK(on_all_chapters_toggled), chapter_box);
    gtk_box_append(GTK_BOX(chapter_box), all_checkbox);
    
    for (int i = 0; i < selection->count; i++) {
        GtkWidget *checkbox = gtk_check_button_new_with_label(selection->chapters[i]);
        g_object_set_data(G_OBJECT(checkbox), "chapter-index", GINT_TO_POINTER(i));
        g_signal_connect(checkbox, "toggled", G_CALLBACK(on_chapter_checkbox_toggled), selection);
        g_signal_connect(checkbox, "toggled", G_CALLBACK(on_individual_chapter_toggled), all_chapters_checkbox);
        gtk_box_append(GTK_BOX(chapter_box), checkbox);
    }
}

static void on_generate_exam_execute_new(GtkWidget *btn, gpointer data) {
    gpointer *params = (gpointer *)data;
    AppData *app = (AppData *)params[0];
    GtkWidget *dialog = (GtkWidget *)params[1];
    GtkWidget *subject_dropdown = (GtkWidget *)params[2];
    ChapterSelection *selection = (ChapterSelection *)params[3];
    GtkWidget *exam_type_dropdown = (GtkWidget *)params[4];
    GtkWidget *file_entry = (GtkWidget *)params[5];
    GtkWidget *format_dropdown = (GtkWidget *)params[6];
    GtkWidget *all_chapters_checkbox = (GtkWidget *)params[7];

    guint subject_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(subject_dropdown));
    const char *subject = COMPUTER_ENGINEERING_SUBJECTS[subject_idx];
    
    gboolean all_chapters = gtk_check_button_get_active(GTK_CHECK_BUTTON(all_chapters_checkbox));
    
    guint exam_type_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(exam_type_dropdown));
    typedef enum { EXAM_TYPE_QCM_ONLY, EXAM_TYPE_MIXED } ExamType;
    ExamType exam_type = (exam_type_idx == 0) ? EXAM_TYPE_QCM_ONLY : EXAM_TYPE_MIXED;
    
    const char *base_filename = gtk_editable_get_text(GTK_EDITABLE(file_entry));
    
    guint format_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(format_dropdown));
    const char *format = (format_idx == 0) ? "TXT" : "PDF";

    if (strlen(subject) == 0) {
        show_notification(app, "Veuillez spécifier une matière", "error");
        return;
    }

    char clean_filename[256];
    strncpy(clean_filename, base_filename, sizeof(clean_filename) - 1);
    clean_filename[sizeof(clean_filename) - 1] = '\0';
    
    char *dot = strrchr(clean_filename, '.');
    if (dot != NULL) {
        if (strcmp(dot, ".txt") == 0 || strcmp(dot, ".pdf") == 0) {
            *dot = '\0';
        }
    }
    
    char final_filename[300];
    if (format_idx == 0) {
        snprintf(final_filename, sizeof(final_filename), "%s.txt", clean_filename);
    } else {
        snprintf(final_filename, sizeof(final_filename), "%s.pdf", clean_filename);
    }

    if (all_chapters) {
        generate_exam(&app->db, subject, "", exam_type, final_filename, format);
    } else {
        int generated_count = 0;
        for (int i = 0; i < selection->count; i++) {
            if (selection->selected[i]) {
                char chapter_filename[300];
                char safe_chapter_name[100];
                strncpy(safe_chapter_name, selection->chapters[i], sizeof(safe_chapter_name) - 1);
                safe_chapter_name[sizeof(safe_chapter_name) - 1] = '\0';
                for (char *p = safe_chapter_name; *p; ++p) {
                    if (*p == ' ') *p = '_';
                }

                snprintf(chapter_filename, sizeof(chapter_filename), "%s_%s", clean_filename, safe_chapter_name);
                if (format_idx == 0) {
                    strcat(chapter_filename, ".txt");
                } else {
                    strcat(chapter_filename, ".pdf");
                }
                generate_exam(&app->db, subject, selection->chapters[i], exam_type, chapter_filename, format);
                generated_count++;
            }
        }
        
        if (generated_count == 0) {
            show_notification(app, "Veuillez sélectionner au moins un chapitre ou cocher 'Tous les chapitres'", "error");
            return;
        }
    }
    
    char notification[256];
    snprintf(notification, sizeof(notification), 
             "Épreuve(s) générée(s) dans 'Epreuves_Generees'");
    show_notification(app, notification, "success");

    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_generate_exam_clicked(GtkWidget *button, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Générer une Épreuve");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 550, 700);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget *main_scroll = gtk_scrolled_window_new();
    gtk_window_set_child(GTK_WINDOW(dialog), main_scroll);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(main_scroll), box);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='x-large' weight='bold'>Paramètres de l'Épreuve</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *subject_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *subject_label = gtk_label_new("Matière");
    gtk_widget_set_halign(subject_label, GTK_ALIGN_START);
    GtkWidget *subject_dropdown = gtk_drop_down_new_from_strings(COMPUTER_ENGINEERING_SUBJECTS);
    gtk_box_append(GTK_BOX(subject_box), subject_label);
    gtk_box_append(GTK_BOX(subject_box), subject_dropdown);
    gtk_box_append(GTK_BOX(box), subject_box);

    GtkWidget *chapter_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *chapter_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(chapter_label), "<span weight='bold'>Chapitres</span>");
    gtk_widget_set_halign(chapter_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(chapter_container), chapter_label);
    
    GtkWidget *chapter_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(chapter_scroll), 150);
    gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(chapter_scroll), 200);
    
    GtkWidget *chapter_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_start(chapter_box, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(chapter_scroll), chapter_box);
    
    GtkWidget *all_chapters_checkbox = gtk_check_button_new_with_label("Tous les chapitres");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(all_chapters_checkbox), TRUE);
    g_signal_connect(all_chapters_checkbox, "toggled", G_CALLBACK(on_all_chapters_toggled), chapter_box);
    gtk_box_append(GTK_BOX(chapter_box), all_chapters_checkbox);
    
    gtk_box_append(GTK_BOX(chapter_container), chapter_scroll);
    gtk_box_append(GTK_BOX(box), chapter_container);

    ChapterSelection *selection = g_new0(ChapterSelection, 1);
    
    gpointer *subject_params = g_new(gpointer, 4);
    subject_params[0] = app;
    subject_params[1] = chapter_box;
    subject_params[2] = selection;
    subject_params[3] = all_chapters_checkbox;
    g_signal_connect(subject_dropdown, "notify::selected", G_CALLBACK(on_subject_changed), subject_params);
    
    on_subject_changed(GTK_DROP_DOWN(subject_dropdown), NULL, subject_params);

    GtkWidget *exam_type_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *exam_type_label = gtk_label_new("Type d'Épreuve");
    gtk_widget_set_halign(exam_type_label, GTK_ALIGN_START);
    
    const char *exam_types[] = {
        "QCM Uniquement (20 questions - 1 pt chacune)",
        "Mixte (10 QCM - 0.5 pt + 1 Exercice - 15 pts)",
        NULL
    };
    GtkWidget *exam_type_dropdown = gtk_drop_down_new_from_strings(exam_types);
    gtk_box_append(GTK_BOX(exam_type_box), exam_type_label);
    gtk_box_append(GTK_BOX(exam_type_box), exam_type_dropdown);
    gtk_box_append(GTK_BOX(box), exam_type_box);

    GtkWidget *format_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *format_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(format_label), "<span weight='bold'>Format de Sortie</span>");
    gtk_widget_set_halign(format_label, GTK_ALIGN_START);
    
    const char *formats[] = {"Fichier Texte (.txt)", "Document PDF (.pdf)", NULL};
    GtkWidget *format_dropdown = gtk_drop_down_new_from_strings(formats);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(format_dropdown), 1);
    gtk_box_append(GTK_BOX(format_box), format_label);
    gtk_box_append(GTK_BOX(format_box), format_dropdown);
    gtk_box_append(GTK_BOX(box), format_box);

    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), 
        "<span size='small' style='italic' foreground='#64748b'>"
        "ℹ️ Toutes les épreuves sont notées sur 20 points"
        "</span>");
    gtk_label_set_wrap(GTK_LABEL(info_label), TRUE);
    gtk_widget_set_halign(info_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), info_label);

    GtkWidget *save_info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(save_info_label), 
        "<span size='small' style='italic' foreground='#64748b'>"
        "📁 Les épreuves seront enregistrées dans le dossier 'Epreuves_Generees'\n"
        "avec un horodatage pour garantir l'unicité des fichiers"
        "</span>");
    gtk_label_set_wrap(GTK_LABEL(save_info_label), TRUE);
    gtk_widget_set_halign(save_info_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), save_info_label);

    GtkWidget *file_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *file_label = gtk_label_new("Nom du Fichier (sans extension)");
    gtk_widget_set_halign(file_label, GTK_ALIGN_START);
    GtkWidget *file_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(file_entry), "epreuve");
    gtk_entry_set_placeholder_text(GTK_ENTRY(file_entry), "Ex: epreuve_programmation");
    gtk_box_append(GTK_BOX(file_box), file_label);
    gtk_box_append(GTK_BOX(file_box), file_entry);
    gtk_box_append(GTK_BOX(box), file_box);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    gtk_widget_set_margin_top(button_box, 10);

    GtkWidget *cancel_btn = gtk_button_new_with_label("Annuler");
    GtkWidget *generate_btn = gtk_button_new_with_label("Générer l'Épreuve");
    gtk_widget_add_css_class(generate_btn, "suggested-action");
    gtk_widget_add_css_class(generate_btn, "pill");

    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    gtk_box_append(GTK_BOX(button_box), generate_btn);
    gtk_box_append(GTK_BOX(box), button_box);

    gpointer *params = g_new(gpointer, 8);
    params[0] = app;
    params[1] = dialog;
    params[2] = subject_dropdown;
    params[3] = selection;
    params[4] = exam_type_dropdown;
    params[5] = file_entry;
    params[6] = format_dropdown;
    params[7] = all_chapters_checkbox;

    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    g_signal_connect(generate_btn, "clicked", G_CALLBACK(on_generate_exam_execute_new), params);
    
    g_signal_connect_swapped(dialog, "destroy", G_CALLBACK(g_free), params);
    g_signal_connect_swapped(dialog, "destroy", G_CALLBACK(g_free), subject_params);
    g_signal_connect_swapped(dialog, "destroy", G_CALLBACK(g_free), selection);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void switch_to_view(AppData *app, const char *view_name) {
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), view_name);
}

static void activate(GtkApplication *gtk_app, gpointer user_data) {
    AppData *app = (AppData *)user_data;

    app->main_window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->main_window), "Générateur d'Épreuves - Ingénierie Informatique");
    gtk_window_set_default_size(GTK_WINDOW(app->main_window), 1100, 750);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(app->main_window), main_box);

    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *header_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_title), 
        "<span weight='bold' size='large'>Générateur d'Épreuves</span>");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), header_title);

    GtkWidget *add_btn = gtk_button_new_from_icon_name("list-add-symbolic");
    gtk_widget_set_tooltip_text(add_btn, "Ajouter une question");
    gtk_widget_add_css_class(add_btn, "circular");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), add_btn);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_question_clicked), app);

    GtkWidget *edit_btn = gtk_button_new_from_icon_name("document-edit-symbolic");
    gtk_widget_set_tooltip_text(edit_btn, "Modifier la question sélectionnée");
    gtk_widget_add_css_class(edit_btn, "circular");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), edit_btn);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_question_clicked), app);

    GtkWidget *delete_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
    gtk_widget_set_tooltip_text(delete_btn, "Supprimer la question sélectionnée");
    gtk_widget_add_css_class(delete_btn, "circular");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), delete_btn);
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_question_clicked), app);

    GtkWidget *generate_btn = gtk_button_new_with_label("Générer Épreuve");
    gtk_widget_add_css_class(generate_btn, "suggested-action");
    gtk_widget_add_css_class(generate_btn, "pill");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), generate_btn);
    g_signal_connect(generate_btn, "clicked", G_CALLBACK(on_generate_exam_clicked), app);

    gtk_box_append(GTK_BOX(main_box), header);

    GtkWidget *list_scroll = gtk_scrolled_window_new();
    app->list_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), app->list_view);
    gtk_box_append(GTK_BOX(main_box), list_scroll);
    gtk_widget_set_vexpand(list_scroll, TRUE);

    GtkWidget *status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(status_bar, 15);
    gtk_widget_set_margin_end(status_bar, 15);
    gtk_widget_set_margin_top(status_bar, 8);
    gtk_widget_set_margin_bottom(status_bar, 8);
    gtk_widget_add_css_class(status_bar, "status-bar");

    app->status_label = gtk_label_new("");
    gtk_widget_set_halign(app->status_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(status_bar), app->status_label);

    char count_text[100];
    snprintf(count_text, sizeof(count_text), "%d questions dans la base", app->db.count);
    app->count_label = gtk_label_new(count_text);
    gtk_widget_set_halign(app->count_label, GTK_ALIGN_END);
    gtk_widget_set_hexpand(app->count_label, TRUE);
    gtk_box_append(GTK_BOX(status_bar), app->count_label);

    gtk_box_append(GTK_BOX(main_box), status_bar);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { "
        "  background-color: #f8fafc; "
        "}"
        ".question-item { "
        "  background: white; "
        "  border-radius: 12px; "
        "  border: 1px solid #e2e8f0; "
        "  cursor: pointer; "
        "  transition: all 0.2s ease; "
        "}"
        ".question-item:hover { "
        "  background: #f1f5f9; "
        "  border-color: #cbd5e1; "
        "  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05); "
        "}"
        ".question-item.selected { "
        "  background: #e0f2fe; "
        "  border-color: #0ea5e9; "
        "  box-shadow: 0 2px 12px rgba(14, 165, 233, 0.15); "
        "}"
        ".badge { "
        "  padding: 6px 14px; "
        "  border-radius: 16px; "
        "  font-size: 0.85em; "
        "  font-weight: bold; "
        "}"
        ".badge-qcm { "
        "  background: linear-gradient(135deg, #dbeafe 0%, #bfdbfe 100%); "
        "  color: #1e40af; "
        "}"
        ".badge-exercice { "
        "  background: linear-gradient(135deg, #fef3c7 0%, #fde68a 100%); "
        "  color: #92400e; "
        "}"
        ".status-bar { "
        "  background: linear-gradient(to right, #f1f5f9 0%, #e2e8f0 100%); "
        "  border-top: 1px solid #cbd5e1; "
        "}"
        "button.circular { "
        "  border-radius: 50%; "
        "  padding: 8px; "
        "}"
        "button.pill { "
        "  border-radius: 20px; "
        "  padding: 8px 20px; "
        "  font-weight: bold; "
        "}"
        "headerbar { "
        "  background: linear-gradient(to right, #1e40af 0%, #3b82f6 100%); "
        "  color: white; "
        "}"
    );
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                                GTK_STYLE_PROVIDER(provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    refresh_question_list(app);
    show_notification(app, "Application prête - Ingénierie Informatique", "info");

    gtk_window_present(GTK_WINDOW(app->main_window));
}

static void on_login_clicked(GtkWidget *button, gpointer user_data) {
    gpointer *params = (gpointer *)user_data;
    AppData *app = (AppData *)params[0];
    GtkWidget *password_entry = (GtkWidget *)params[1];
    GtkWidget *error_label = (GtkWidget *)params[2];
    
    const char *entered_password = gtk_editable_get_text(GTK_EDITABLE(password_entry));
    
    if (strcmp(entered_password, PASSWORD) == 0) {
        gtk_window_destroy(GTK_WINDOW(app->login_window));
        activate(app->gtk_app, app);
    } else {
        gtk_label_set_markup(GTK_LABEL(error_label), 
            "<span foreground='#ef4444' weight='bold'>Mot de passe incorrect</span>");
    }
}

static void show_login_window(GtkApplication *gtk_app, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    app->gtk_app = gtk_app;
    
    app->login_window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->login_window), "Connexion - Générateur d'Épreuves");
    gtk_window_set_default_size(GTK_WINDOW(app->login_window), 450, 350);
    gtk_window_set_resizable(GTK_WINDOW(app->login_window), FALSE);
    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(app->login_window), main_box);
    
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(header_box, 40);
    gtk_widget_set_margin_end(header_box, 40);
    gtk_widget_set_margin_top(header_box, 50);
    gtk_widget_set_margin_bottom(header_box, 30);
    gtk_widget_add_css_class(header_box, "login-header");
    
    GtkWidget *app_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(app_title), 
        "<span size='xx-large' weight='bold'>Générateur d'Épreuves</span>");
    gtk_widget_set_halign(app_title, GTK_ALIGN_CENTER);
    
    GtkWidget *app_subtitle = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(app_subtitle), 
        "<span size='large' foreground='#64748b'>Ingénierie Informatique</span>");
    gtk_widget_set_halign(app_subtitle, GTK_ALIGN_CENTER);
    
    gtk_box_append(GTK_BOX(header_box), app_title);
    gtk_box_append(GTK_BOX(header_box), app_subtitle);
    gtk_box_append(GTK_BOX(main_box), header_box);
    
    GtkWidget *form_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(form_box, 40);
    gtk_widget_set_margin_end(form_box, 40);
    gtk_widget_set_margin_bottom(form_box, 40);
    
    GtkWidget *password_label = gtk_label_new("Mot de passe");
    gtk_widget_set_halign(password_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(password_label, "login-label");
    
    GtkWidget *password_entry = gtk_password_entry_new();
    gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(password_entry), TRUE);
    gtk_widget_add_css_class(password_entry, "login-entry");
    gtk_widget_set_size_request(password_entry, -1, 45);
    
    GtkWidget *error_label = gtk_label_new("");
    gtk_widget_set_halign(error_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(error_label, 5);
    
    GtkWidget *login_button = gtk_button_new_with_label("Se connecter");
    gtk_widget_add_css_class(login_button, "login-button");
    gtk_widget_set_size_request(login_button, -1, 45);
    
    gtk_box_append(GTK_BOX(form_box), password_label);
    gtk_box_append(GTK_BOX(form_box), password_entry);
    gtk_box_append(GTK_BOX(form_box), error_label);
    gtk_box_append(GTK_BOX(form_box), login_button);
    
    gtk_box_append(GTK_BOX(main_box), form_box);
    
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), 
        "<span size='small' foreground='#94a3b8'>Mot de passe par défaut : 12345</span>");
    gtk_widget_set_halign(info_label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(info_label, 20);
    gtk_box_append(GTK_BOX(main_box), info_label);
    
    gpointer *params = g_new(gpointer, 3);
    params[0] = app;
    params[1] = password_entry;
    params[2] = error_label;
    
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_clicked), params);
    g_signal_connect_swapped(app->login_window, "destroy", G_CALLBACK(g_free), params);
    
    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect_swapped(key_controller, "key-pressed", 
        G_CALLBACK(gtk_widget_activate), login_button);
    gtk_widget_add_controller(password_entry, key_controller);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { "
        "  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); "
        "}"
        ".login-header { "
        "  background: white; "
        "  border-radius: 16px; "
        "  padding: 20px; "
        "  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.1); "
        "}"
        ".login-label { "
        "  font-weight: bold; "
        "  font-size: 14px; "
        "  color: #1e293b; "
        "}"
        ".login-entry { "
        "  border-radius: 8px; "
        "  border: 2px solid #e2e8f0; "
        "  padding: 12px; "
        "  font-size: 14px; "
        "}"
        ".login-entry:focus { "
        "  border-color: #667eea; "
        "  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1); "
        "}"
        ".login-button { "
        "  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); "
        "  color: white; "
        "  border-radius: 8px; "
        "  font-weight: bold; "
        "  font-size: 15px; "
        "  border: none; "
        "}"
        ".login-button:hover { "
        "  background: linear-gradient(135deg, #5568d3 0%, #6a3f8f 100%); "
        "  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4); "
        "}"
    );
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                                GTK_STYLE_PROVIDER(provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    gtk_window_present(GTK_WINDOW(app->login_window));
}

int main(int argc, char **argv) {
    srand(time(NULL));

    AppData app = {0};
    app.selected_question_index = -1;
    load_database(&app.db, DB_FILE);

    GtkApplication *gtk_app = gtk_application_new("com.generateur.epreuve.informatique", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(show_login_window), &app);

    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);

    free_database(&app.db);
    g_object_unref(gtk_app);

    return status;
}
