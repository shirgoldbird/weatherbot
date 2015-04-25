#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LinkedList.h"
#include "frozen.h"

//compile with cc -g frozen.c api_test.c -o api_test -lcurl

// frozen library from https://github.com/cesanta/frozen
// cURL writer functions from http://stackoverflow.com/questions/2329571/c-libcurl-get-output-into-a-string

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

// from http://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c

char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

char* get_weather(char *zipcode);
void save_json(struct json_token *arr, LinkedListPtr list);
void parse_sentence(char *sentence, LinkedListPtr list);
Data make_data(struct json_token *tok);

int main(void){
  LinkedListPtr weather = initLinkedList();
  struct json_token *arr, *tok, *loc, *city, *state;
  char *json, *results, location[100], zipcode[6] = "02148\0", command[256];
  char *token;
  char *end_str;
  int done;

  printf("-- Welcome to WEATHERBOT, your personal weather assistant! --\n");
/*  printf("\nPlease enter your zip code: ");
  fgets(zipcode, 7, stdin);
  strtok(zipcode, "\n"); */

  json = get_weather(zipcode);
  arr = parse_json2(json, strlen(json));
  // Parse out and save the weather in a linked list
  save_json(arr, weather);

  // Figure out where in the world this weather is
  loc = find_json_token(arr, "query.results.channel.location");
  city = find_json_token(loc, "city");
  state = find_json_token(loc, "region");

  printf("Okay, I got the five day forecast for %.*s, %.*s for you.\n", city->len, city->ptr, state->len, state->ptr);

  while(!done){
    printf("\nWhat would you like to know? (type a command, 'help', or 'quit') ");
    fgets(command, 256, stdin);
    strtok(command, "\n");
    if(strcmp(command, "help") == 0){
      printf("\nSample commands (try one out!):\nWhat's the weather going to be like Tuesday?\nHow warm will Friday be?\nWhat's the [high/low] for tomorrow?\n");
    }
    else if (strcmp(command, "quit") == 0){
      printf("It was a pleasure to assist you. Enjoy the weather!\n");
      done = 1;
    }
    else{
      parse_sentence(command, weather);
    }
  }
  empty(weather);
  destroy(weather);
  free(arr);
  return 0;
}

char* get_weather(char *zipcode){
  CURL *curl;
  CURLcode res;
  struct string s;
  char *json;
  char *urlstart = "https://query.yahooapis.com/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20woeid%20in%20%28select%20woeid%20from%20geo.places%281%29%20where%20text%3D%22";
  char *urlend = "%2Cusa%22%29&format=json&env=store%3A%2F%2Fdatatables.org%2Falltableswithkeys";
  char newurl[2000];

  // Tokenize json string, fill in tokens array
  init_string(&s);
  curl = curl_easy_init();

  // Build the url with the given zipcode
  strcpy(newurl, urlstart);
  strcat(newurl, zipcode);
  strcat(newurl, urlend);

  if(curl) {
    struct string s;
    init_string(&s);

    curl_easy_setopt(curl, CURLOPT_URL, newurl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    res = curl_easy_perform(curl);

    json = s.ptr;

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  free(s.ptr);
  return json;
}

void save_json(struct json_token *arr, LinkedListPtr list){ // Parse the big ugly json Yahoo returns and save it to a linked list
  char *holder, *token, *end_str, *spare, *spare2;
  struct json_token *tok, *date_weather;
  Data day;
  int i = 0;

  // Pull out the actual weather
  tok = find_json_token(arr, "query.results.channel.item.forecast");

  holder = strdup(tok->ptr);
  token = strtok_r(holder, "[{}", &end_str);
  while (token != NULL){
      char *end_token;
      // DON'T process the unneeded tokens
      if(strcmp(token,",") != 0 && strcmp(token,",\"guid\":") != 0 && strncmp(token, "\"isPermaLink\"", 13) != 0){
        printf("\na = %s\n", token);

        spare = str_replace(token, "\"", "");
        spare2 = str_replace(spare, ":", ": \"");
        spare = str_replace(spare2, ",", "\", ");
        spare2 = str_replace(spare, "code", "{code");

        // add back in some formatting that got eaten
        size_t len = strlen(spare2);
        spare = malloc(len + 2);
        strcpy(spare, spare2);
        spare[len] = '\"';
        spare[len + 1] = '}';
        spare[len + 2] = '\0';

        date_weather = parse_json2(spare, strlen(spare));
        // extract the data for each day
        day = make_data(date_weather);
        // add it to the linked list
        add(list, day, i);
        i++;
      }
  }
}

void parse_sentence(char *sentence, LinkedListPtr list){ // Go through user input to see if they mentioned a day and a command
  int i, found = 0;
  char *the_day, *the_command;
  char *word = strtok (sentence," ,.-?");
  char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday","Thursday","Friday","Saturday","today","tomorrow", 0};
  char *commands[] = {"weather", "temperature", "high", "low", "warm", "cold", "forecast", 0};
  while (word != NULL){
    i = 0;
    while(days[i]){
      if(strcmp(days[i], word) == 0){ // Found a valid day
        the_day = word;
        printf("Found day: %s\n", the_day);
        found++;
        break;
      }
      i++;
    }
    i = 0;
    while(commands[i]){
      if(strcmp(commands[i], word) == 0){ // Found a valid command
        the_command = word;
        printf("Found command: %s\n", the_command);
        found++;
        break;
      }
      i++;
    }
    word = strtok (NULL, " ,.-?");
  }
  if(found != 2){ // Nothing doing
    printf("Sorry, I didn't understand that. Give it another try?\n");
  }
  else{ // There's both a command and a day so we fetch the appropriate information
    
  }
}

Data make_data(struct json_token *tok){ // Takes the weather for one day and makes a data struct from it
  Data data;
  char temp_holder[10];
  struct json_token *holder;

  holder = find_json_token(tok, "date");
  printf("Date: %.*s\n", holder->len, holder->ptr);
  data.date = strndup(holder->ptr, holder->len);
  holder = find_json_token(tok, "day");
  data.day = strndup(holder->ptr, holder->len);
  holder = find_json_token(tok, "high");
  strncpy(temp_holder, holder->ptr, holder->len);
  data.high = atoi(temp_holder);
  holder = find_json_token(tok, "low");
  strncpy(temp_holder, holder->ptr, holder->len);
  data.low = atoi(temp_holder);
  holder = find_json_token(tok, "text");
  data.description = strndup(holder->ptr, holder->len);

  return data;
}
