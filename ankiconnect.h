
typedef size_t (*ResponseFunction)(char *ptr, size_t size, size_t nmemb, void *userdata);

typedef struct {
  char *deck;
  char *notetype;

  size_t num_fields;
  char **fieldnames;
  char **fieldentries;
} ankicard;

const char *search_query(const char *query, ResponseFunction respf);
const char *addNote(ankicard ac);
