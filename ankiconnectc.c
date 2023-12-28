#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <glib.h>

#include "ankiconnectc.h"

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

typedef size_t (*InternalResponseFunction)(char *ptr, size_t size, size_t nmemb, void *userdata);

ankicard*
new_ankicard()
{
	ankicard *ac = malloc(sizeof(ankicard));

	ac->deck = NULL;
	ac->notetype = NULL;
	ac->num_fields = -1;
	ac->fieldnames = NULL;
	ac->fieldentries = NULL;
	ac->tags = NULL;

	return ac;
}

void
ac_print_anki_card(ankicard ac)
{
	printf("Deck name: %s\n", ac.deck);
	printf("Notetype: %s\n", ac.notetype);
	for (int i = 0; i < ac.num_fields; i++)
		printf("%s: %s\n", ac.fieldnames[i], ac.fieldentries[i] == NULL ? "" : ac.fieldentries[i]);
}

size_t
check_add_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	AddResponseFunction *user_function = userdata;
	if (!user_function) return nmemb;

	const char *err_str = "\"error\": ";
	char *err_start = strstr(ptr, err_str);
	err_start = err_start ? err_start + strlen(err_str) : NULL;

	if (strstr(ptr, "\"error\": null"))
		(*user_function)(NULL);
	else
		(*user_function)(ptr);

	return nmemb;
}

const char *
sendRequest(char *request, InternalResponseFunction internal_respfunc, void* user_func)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return "Error initialiting cURL";

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");

	const char *env_url = getenv(AC_API_URL_EVAR);
	if (env_url)
		curl_easy_setopt(curl, CURLOPT_URL, env_url);
	else
		curl_easy_setopt(curl, CURLOPT_URL, DEFAULT_AC_API_URL);

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal_respfunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, user_func);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
		return curl_easy_strerror(res);

	curl_easy_cleanup(curl);
	return NULL;
}


size_t
search_check_function(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	SearchResponseFunction *user_func = userdata;
	if (!user_func) return nmemb;

	int result = (strncmp(ptr, "{\"result\": [], \"error\": null}", nmemb) != 0);
	(*user_func)(result); // Fork this maybe? Might be bad if function doesn't quit.

	return nmemb;
}

/*
 * @entry: The entry to search for
 * @field: The field where @entry should be searched in
 * @deck: The deck to search in. Can be null.
 *
 * Returns:
 */
const char *
ac_search(const char* deck, const char* field, const char* entry, SearchResponseFunction user_function)
{
	char *request;

	asprintf(&request, "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : \"\\\"%s%s\\\" \\\"%s:%s\\\"\" } }",
		 deck ? "deck:" : "", deck ? deck : "", field, entry);

	const char *err = sendRequest(request, search_check_function, &user_function);
	free(request);
	return err;
}

const char *
ac_action_query(const char *action, const char *query)
{
	char *request;

	asprintf(&request, "{ \"action\": \"%s\", \"version\": 6, \"params\": { \"query\" : \"%s\" } }", action, query);

	const char *err = sendRequest(request, NULL, NULL);
	free(request);
	return err;
}

char*
json_esc_str(const char *str)
{
	if (!str)
		return NULL;

	GString *gstr = g_string_sized_new(strlen(str) * 2);

	do{
		switch (*str)
		{
		case '\n':
			g_string_append(gstr, "<br>");
			break;

		case '\t':
			g_string_append(gstr, "\\\\t");
			break;

		case '"':
			g_string_append(gstr, "\\\"");
			break;

		case '\\':
			g_string_append(gstr, "\\\\");
			break;

		default:
			g_string_append_c(gstr, *str);
		}
	} while (*str++);

	return g_string_free_and_steal(gstr);
}

void
json_esc_ac(const ankicard ac, ankicard *ac_je)
{
	/* Creates a json escaped and \n -> <br> converted copy of the anki card. */
	ac_je->deck = json_esc_str(ac.deck);
	ac_je->notetype = json_esc_str(ac.notetype);

	ac_je->num_fields = ac.num_fields;
	ac_je->fieldnames = calloc(ac_je->num_fields, sizeof(char *));
	ac_je->fieldentries = calloc(ac_je->num_fields, sizeof(char *));

	for (int i = 0; i < ac.num_fields; i++)
	{
		ac_je->fieldnames[i] = json_esc_str(ac.fieldnames[i]);
		ac_je->fieldentries[i] = json_esc_str(ac.fieldentries[i]);
	}
}

void
free_ankicard(ankicard ac)
{
	g_free(ac.deck);
	g_free(ac.notetype);

	for (int i = 0; i < ac.num_fields; i++)
	{
		g_free(ac.fieldnames[i]);
		g_free(ac.fieldentries[i]);
	}

	free(ac.fieldnames);
	free(ac.fieldentries);
}

const char*
check_card(const ankicard ac)
{
	return ac.num_fields == -1 ? "ERROR: Number of fields integer not specified in anki card."
	     : !ac.deck ? "ERROR : No deck specified."
	     : !ac.notetype ? "ERROR : No notetype specified."
	     : !ac.fieldnames ? "ERROR : No fieldnames provided."
	     : !ac.fieldentries ? "ERROR : No fieldentries provided."
	     : NULL;
}

const char*
ac_addNote(const ankicard ac, AddResponseFunction user_func)
{
	const char *err = check_card(ac);
	if (err) return err;

	ankicard ac_je;
	json_esc_ac(ac, &ac_je);

	GString* request = g_string_sized_new(500);
	g_string_printf(request,
			"{"
			"\"action\": \"addNote\","
			"\"version\": 6,"
			"\"params\": {"
			"\"note\": {"
			"\"deckName\": \"%s\","
			"\"modelName\": \"%s\","
			"\"fields\": {",
			ac_je.deck,
			ac_je.notetype
			);

	for (int i = 0; i < ac_je.num_fields; i++)
	{
		g_string_append_printf(request,
				       "\"%s\": \"%s\"",
				       ac_je.fieldnames[i],
				       ac_je.fieldentries[i] == NULL ? "" : ac_je.fieldentries[i]
				       );
		if (i < ac_je.num_fields - 1)
			g_string_append_c(request, ',');
	}

	g_string_append(request,
			"},"
			"\"options\": {"
			"\"allowDuplicate\": true"
			"},"
			"\"tags\": []"
			"}"
			"}"
			"}"
			);

	err = sendRequest(request->str, check_add_response, &user_func);

	free_ankicard(ac_je);
	g_string_free(request, TRUE);
	return err;
}
