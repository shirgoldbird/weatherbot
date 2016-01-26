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

LinkedListPtr get_weather();
void save_json(struct json_token *arr, LinkedListPtr list);
LinkedListPtr parse_sentence(char *sentence, LinkedListPtr list);
Data make_data(struct json_token *tok);
char *parse_day(char *original_date);
char *parse_command(Data day_data, char *the_command);

int main(int argc, char *argv[]){
  LinkedListPtr weather = initLinkedList();
  struct json_token *arr, *tok, *loc, *city, *state;
  char *json, *results, location[100], command[256];
  char *token;
  char *end_str;
  int done;
  Data day_data;

  printf("-- Welcome to WEATHERBOT, your personal weather assistant! --\n");

  weather = get_weather();

  while(!done){
    printf("\nWhat would you like to know? (type a command, 'new zip', 'help', or 'quit') ");
    fgets(command, 256, stdin);
    strtok(command, "\n");
    if(strcasecmp(command, "help") == 0){
      printf("\nSample commands (try one out!):\nWhat's the weather going to be like Tuesday?\nHow warm will Friday be?\nWhat's the [high/low] for tomorrow?\n");
    }
    else if (strcasecmp(command, "quit") == 0){
      printf("It was a pleasure to assist you. Enjoy the weather!\n");
      done = 1;
    }
    else{
      weather = parse_sentence(command, weather);
    }
  }
  empty(weather);
  destroy(weather);
  free(arr);
  return 0;
}

LinkedListPtr get_weather(){
  CURL *curl;
  CURLcode res;
  struct string s;
  char *json, zipcode[7];
  char *urlstart = "https://query.yahooapis.com/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20woeid%20in%20%28select%20woeid%20from%20geo.places%281%29%20where%20text%3D%22";
  char *urlend = "%2Cusa%22%29&format=json&env=store%3A%2F%2Fdatatables.org%2Falltableswithkeys";
  char newurl[2000];
  LinkedListPtr weather = initLinkedList();
  struct json_token *arr, *loc, *city, *state;

  printf("\nPlease enter your zip code: ");
  fgets(zipcode, 7, stdin);
  strtok(zipcode, "\n");

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

  arr = parse_json2(json, strlen(json));
  // Parse out and save the weather in a linked list
  save_json(arr, weather);

  // Figure out where in the world this weather is
  loc = find_json_token(arr, "query.results.channel.location");
  city = find_json_token(loc, "city");
  state = find_json_token(loc, "region");

  printf("Okay, I got the five day forecast for %.*s, %.*s for you.\n", city->len, city->ptr, state->len, state->ptr);

  return weather;
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
    if(strcasecmp(token,",") != 0 && strcasecmp(token,",\"guid\":") != 0 && strncmp(token, "\"isPermaLink\"", 13) != 0){

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
    token = strtok_r(NULL, "[{}]", &end_str);
  }
}

LinkedListPtr parse_sentence(char *sentence, LinkedListPtr list){ // Go through user input to see if they mentioned a day and a command, print appropriate response.
  // This should really be two functions but I just don't care enough
  int i, found_cmd = 0, found_day = 0;
  Data day_data;
  day_data.high = 420; // Should probably be some more reasonable sentinal value but the joke was too hard to pass up
  char *the_day, *the_command, *day_abbreviation;
  char *word = strtok(sentence," ,.-?");
  char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday","Thursday","Friday","Saturday","today","tomorrow", 0};
  char *commands[] = {"weather", "temperature", "high", "low", "warm", "cold", "forecast", "temp", "zip", 0};
  LinkedListPtr new_list = initLinkedList();

  while (word != NULL){
    i = 0;
    while(days[i]){
      if(strcasecmp(days[i], word) == 0){ // Found a valid day
        the_day = days[i];
        found_day++;
        break;
      }
      i++;
    }
    i = 0;
    while(commands[i]){
      if(strcasecmp(commands[i], word) == 0){ // Found a valid command
        if(strcasecmp(word, "warm") == 0 || strcasecmp(word, "cold") == 0 || strcasecmp(word, "temp") == 0){ // This is needed for sensible sentence printing later. Prob in the wrong place.
          the_command = "temperature";
        }
        else if(strcasecmp(word, "zip") == 0){ // User wants to enter a new zip code
          new_list = get_weather();
          return new_list;
        }
        else{
          the_command = word;
        }
        found_cmd++;
        break;
      }
      i++;
    }
    word = strtok(NULL, " ,.-?");
  }
  if(found_cmd != 1){ // Nothing doing
    printf("Sorry, I didn't understand that. Give it another try?\n");
  }
  else{ // There's both a command and a day so we fetch the appropriate information
    if((found_day == 0) || (strcasecmp(the_day, "today") == 0)){ // let's do the easy ones first
      day_data = check(list, 0);
      the_day = "today";
    }
    else if(strcasecmp(the_day, "tomorrow") == 0){
      day_data = check(list, 1);
    }
    else{
      day_abbreviation = parse_day(the_day);
      if(strcasecmp(day_abbreviation, "ERROR") != 0){ // Valid day abbreviation found
        for(int i = 0; i < 5; i++){ // go through all five items in linked list
          if(strcasecmp(day_abbreviation, check(list, i).day) == 0){ // did we hit the right day?
            day_data = check(list, i);
            break;
          }
        }
      }
      else{
        printf("Sorry, I ran into an error. Try asking something else?\n");
      }
    }
    if(day_data.high == 420){
      printf("Sorry, I don't have the forecast for %s yet! Ask me later.\n", the_day);
    }
    else if(the_day == NULL){
      printf("The %s for today is %s.\n", the_command, parse_command(day_data, the_command)); // FINALLY PRINT THE GD RESULT
    }
    else{
      printf("The %s for %s is %s.\n", the_command, the_day, parse_command(day_data, the_command)); // FINALLY PRINT THE GD RESULT
    }
  }
  return list;
}

Data make_data(struct json_token *tok){ // Takes the weather for one day and makes a data struct from it
  Data data;
  char temp_holder[10];
  struct json_token *holder;

  holder = find_json_token(tok, "date");
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

char *parse_day(char *original_date){
  if(strcasecmp(original_date, "Sunday") == 0){
    return "Sun";
  }
  else if(strcasecmp(original_date, "Monday") == 0){
    return "Mon";
  }
  else if(strcasecmp(original_date, "Tuesday") == 0){
    return "Tue";
  }
  else if(strcasecmp(original_date, "Wednesday") == 0){
    return "Wed";
  }
  else if(strcasecmp(original_date, "Thursday") == 0){
    return "Thu";
  }
  else if(strcasecmp(original_date, "Friday") == 0){
    return "Fri";
  }
  else if(strcasecmp(original_date, "Saturday") == 0){
    return "Sat";
  }
  else{
    return "ERROR";
  }
}

char *parse_command(Data day_data, char *the_command){
  char *commands[] = {"weather", "temperature", "high", "low", "warm", "cold", "forecast", 0};
  char *temperature = malloc(50), *high = malloc(10), *low = malloc(10);
  if(strcasecmp(the_command, "weather") == 0 || strcasecmp(the_command, "forecast") == 0){
    return day_data.description;
  }
  else if(strcasecmp(the_command, "temperature") == 0){
    sprintf(temperature, "a high of %d and a low of %d", day_data.high, day_data.low);
    return temperature;
  }
  else if(strcasecmp(the_command, "high") == 0){
    sprintf(temperature, "%d", day_data.high);
    return temperature;
  }
  else if(strcasecmp(the_command, "low") == 0){
    sprintf(temperature, "%d", day_data.low);
    return temperature;
  }
  else{
    return "ERROR";
  }
}
