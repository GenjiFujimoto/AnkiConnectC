


/* 
 * @exists contains = 1 on successfull search and 0 otherwise 
 */
typedef void (*SearchResponseFunction)(int exists);

/*
 * @err: Contains an error description on error and NULL otherwise.
 */
typedef void (*AddResponseFunction)(const char *err);

typedef struct {
  char *deck;
  char *notetype;

  int num_fields;
  char **fieldnames;
  char **fieldentries;
  char *tags; //space separated list
} ankicard;

/* 
 * Create an anki card with entries initialized to NULL and num_fields set to -1. Needs to be freed. 
 */
ankicard* new_ankicard();

/* Perform a database search. Result is passed to the response function. */
const char * ac_search(const char* deck, const char* field, const char* entry, SearchResponseFunction user_function);

const char *ac_action_query(const char *action, const char *query);

/* Add ankicard to anki  */
const char* ac_addNote(const ankicard ac, AddResponseFunction user_func);

/* Print contents of an anki card */
void ac_print_anki_card(ankicard ac);
